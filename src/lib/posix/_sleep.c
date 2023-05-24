#include <lib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

static void handler(int sig) {
	/* Dummy signal handler. */
}

unsigned int sleep(unsigned sleepSeconds) {
	sigset_t ssFull, ssOrig, ssAlarm;
	struct sigaction actionAlarm, actionOrig;
	unsigned alarmSeconds, napSeconds;

	if (sleepSeconds == 0)
	  return 0;		/* No reset for the wicked */

	/* Mask all signals. */
	sigfillset(&ssFull);
	sigprocmask(SIG_BLOCK, &ssFull, &ssOrig);

	/* Cancel currently running alarm. */
	alarmSeconds = alarm(0);

	/* How long can we nap withour interruptions? */
	napSeconds = sleepSeconds;
	if (alarmSeconds != 0 && alarmSeconds < sleepSeconds) 
	  napSeconds = alarmSeconds;

	/* Now sleep. */
	actionAlarm.sa_handler = handler;
	sigemptyset(&actionAlarm.sa_mask);
	actionAlarm.sa_flags = 0;
	sigaction(SIGALRM, &actionAlarm, &actionOrig);
	alarm(napSeconds);

	/* Wait for a wakeup call, either our alarm, or some other signal. */
	ssAlarm = ssOrig;
	sigdelset(&ssAlarm, SIGALRM);
	sigsuspend(&ssAlarm);
	
	/* Cancel alarm, set mask and stuff back to normal. */
	napSeconds -= alarm(0);
	sigaction(SIGALRM, &actionOrig, NULL);
	sigprocmask(SIG_SETMASK, &ssOrig, NULL);

	/* Restore alarm counter to the time remaining. */
	if (alarmSeconds != 0 && alarmSeconds >= napSeconds) {
		alarmSeconds -= napSeconds;
		if (alarmSeconds == 0) 
		  raise(SIGALRM);		/* Alarm expires now! */
		else
		  alarm(alarmSeconds);	/* Count time remaining. */
	}

	/* Return time not slept. */
	return sleepSeconds - napSeconds;
}

