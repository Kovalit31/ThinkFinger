/*   ThinkFinger Pluggable Authentication Module
 *
 *   PAM module for libthinkfinger which is a driver for the UPEK/SGS Thomson
 *   Microelectronics fingerprint reader.
 *
 *   Copyright (C) 2007 Timo Hoenig <thoenig@suse.de>, <thoenig@nouse.net>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <config.h>

#include <libthinkfinger.h>
#include <pam_thinkfinger-uinput.h>

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>
#include <syslog.h>
#include <security/pam_modules.h>
#ifdef HAVE_OLD_PAM
#include "pam_thinkfinger-compat.h"
#else
#include <security/pam_ext.h>
#endif

#define MAX_PATH    256

#define PAM_SM_AUTH

struct pam_thinkfinger_s {
	libthinkfinger *tf;
	const char *user;
	pthread_t t_pam_prompt;
	pthread_t t_thinkfinger;
	int swipe_retval;
	int prompt_retval;
	int isatty;
	int uinput_fd;
	pam_handle_t *pamh;
};

static int pam_thinkfinger_check_user (const char *user)
{
	int retval = -1;
	int fd;
	char bir_file[MAX_PATH];

	snprintf (bir_file, MAX_PATH-1, "%s/%s.bir", PAM_BIRDIR, user);
	fd = open (bir_file, O_RDONLY);
	if (fd == -1)
		goto out;

	retval = 0;
	close (fd);

out:
	return retval;
}

static libthinkfinger_state pam_thinkfinger_verify (const struct pam_thinkfinger_s *pam_thinkfinger)
{
	libthinkfinger_state tf_state = TF_STATE_VERIFY_FAILED;
	char bir_file[MAX_PATH];

	snprintf (bir_file, MAX_PATH, "%s/%s.bir", PAM_BIRDIR, pam_thinkfinger->user);

	if (pam_thinkfinger->tf == NULL)
		goto out;

	libthinkfinger_set_file (pam_thinkfinger->tf, bir_file);
	tf_state = libthinkfinger_verify (pam_thinkfinger->tf);

out:
	return tf_state;
}

static void thinkfinger_thread (void *data)
{
	struct pam_thinkfinger_s *pam_thinkfinger = data;
	libthinkfinger_state tf_state;

	pam_thinkfinger->swipe_retval = PAM_SERVICE_ERR;
	tf_state = pam_thinkfinger_verify (pam_thinkfinger);
	if (tf_state == TF_RESULT_VERIFY_SUCCESS) {
		pam_thinkfinger->swipe_retval = PAM_SUCCESS;
		pam_syslog (pam_thinkfinger->pamh, LOG_NOTICE,
			    "%s authenticated (biometric identification record matched)",
			    pam_thinkfinger->user);
	} else if (tf_state == TF_RESULT_VERIFY_FAILED) {
		pam_thinkfinger->swipe_retval = PAM_AUTH_ERR;
		pam_syslog (pam_thinkfinger->pamh, LOG_NOTICE,
			    "%s verication failed (biometric identification record not matched)",
			    pam_thinkfinger->user);
	} else {
		pam_thinkfinger->swipe_retval = PAM_AUTH_ERR;
		pam_syslog (pam_thinkfinger->pamh, LOG_NOTICE,
			    "%s verication failed (0x%x)",
			    pam_thinkfinger->user, tf_state);
		goto out;
	}

	if (uinput_cr (&pam_thinkfinger->uinput_fd) < 0)
		pam_syslog (pam_thinkfinger->pamh, LOG_ERR, "Could not send carriage return via uinput");
out:
	pthread_exit (NULL);
}

static void pam_prompt_thread (void *data)
{
	struct pam_thinkfinger_s *pam_thinkfinger = data;
	char *resp;

	pam_prompt (pam_thinkfinger->pamh, PAM_PROMPT_ECHO_OFF, &resp, "Password or swipe finger: ");
	pam_set_item (pam_thinkfinger->pamh, PAM_AUTHTOK, resp);
	pthread_cancel (pam_thinkfinger->t_thinkfinger);

	pthread_exit (NULL);
}

static const char *handle_error (libthinkfinger_init_status init_status)
{
	const char *msg;

	switch (init_status) {
	case TF_INIT_NO_MEMORY:
		msg = "Not enough memory.";
		break;
	case TF_INIT_USB_DEVICE_NOT_FOUND:
		msg = "USB device not found.";
		break;
	case TF_INIT_USB_OPEN_FAILED:
		msg = "Could not open USB device.";
		break;
	case TF_INIT_USB_CLAIM_FAILED:
		msg = "Could not claim USB device.";
		break;
	case TF_INIT_USB_HELLO_FAILED:
		msg = "Sending HELLO failed.";
		break;
	case TF_INIT_UNDEFINED:
		msg = "Undefined error.";
		break;
	default:
		msg = "Unknown error.";
	}	

	return msg;
}

PAM_EXTERN
int pam_sm_authenticate (pam_handle_t *pamh, int flags, int argc, const char **argv)
{
	int retval = PAM_SERVICE_ERR;
	struct pam_thinkfinger_s pam_thinkfinger;
	struct termios term_attr;
	libthinkfinger_init_status init_status;

	pam_thinkfinger.isatty = isatty (STDIN_FILENO);
	if (pam_thinkfinger.isatty == 1)
		tcgetattr (STDIN_FILENO, &term_attr);

	pam_get_user (pamh, &pam_thinkfinger.user, NULL);
	if (pam_thinkfinger_check_user (pam_thinkfinger.user) < 0) {
		retval = PAM_USER_UNKNOWN;
		goto out;
	}

	if (uinput_open (&pam_thinkfinger.uinput_fd) < 0) {
		pam_syslog (pamh, LOG_ERR, "Initializing uinput failed.");
		retval = PAM_IGNORE;
		goto out;
	}

	pam_thinkfinger.tf = libthinkfinger_new (&init_status);
	if (init_status != TF_INIT_SUCCESS) {
		pam_syslog (pamh, LOG_ERR, "%s", handle_error (init_status));
		retval = PAM_IGNORE;
		goto out;
	}

	pam_thinkfinger.pamh = pamh;
	pthread_create (&pam_thinkfinger.t_thinkfinger, NULL, (void *) &thinkfinger_thread, &pam_thinkfinger);
	pthread_create (&pam_thinkfinger.t_pam_prompt, NULL, (void *) &pam_prompt_thread, &pam_thinkfinger);
	pthread_join (pam_thinkfinger.t_pam_prompt, NULL);
	pthread_join (pam_thinkfinger.t_thinkfinger, NULL);

	if (pam_thinkfinger.tf)
		libthinkfinger_free (pam_thinkfinger.tf);
	if (pam_thinkfinger.uinput_fd > 0)
		uinput_close (&pam_thinkfinger.uinput_fd);
	if (pam_thinkfinger.isatty == 1) {
		tcsetattr (STDIN_FILENO, TCSADRAIN, &term_attr);
	}

	if (pam_thinkfinger.swipe_retval == PAM_SUCCESS)
		retval = PAM_SUCCESS;
	else
		retval = PAM_SERVICE_ERR;
out:
	return retval;
}

PAM_EXTERN
int pam_sm_setcred (pam_handle_t *pamh, int flags, int argc, const char **argv)
{
	return PAM_SUCCESS;
}

PAM_EXTERN
int pam_sm_chauthtok (pam_handle_t *pamh, int flags, int argc, const char **argv)
{
	return PAM_SUCCESS;
}

#ifdef PAM_STATIC

struct pam_module _pam_thinkfinger_modstruct = {
	"pam_thinkfinger",
	pam_sm_authenticate,
	pam_sm_setcred,
	NULL,
	NULL,
	NULL,
	pam_sm_chauthtok
};

#endif
