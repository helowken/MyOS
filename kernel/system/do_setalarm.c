#include "../system.h"

static void causeAlarm(Timer *tp) {
/* Routine called if a timer goes off and the process requested a synchronous
 * alarm. The process number is stored in timer argument 'ta_int'. Notify that
 * process with notification message from CLOCK.
 */
	int pNum = timerArg(tp)->ta_int;	/* Get process number */
	lockNotify(CLOCK, pNum);		/* Notify process */
}

int doSetAlarm(Message *msg) {
/* A process requests a synchronous alarm, or wants to cancel its alarm. */
	register Proc *rp;
	int pNum;
	long expTime;
	int useAbsTime;
	Timer *tp;
	clock_t uptime;

	expTime = msg->ALARM_EXP_TIME;		/* Alarm's expiration time */
	useAbsTime = msg->ALARM_ABS_TIME;	/* Flag for absolute time */
	pNum = msg->m_source;		/* Process to interrupt later */
	rp = procAddr(pNum);
	if (! (priv(rp)->s_flags & SYS_PROC))
	  return EPERM;

	/* Get the timer structure and set the parameters for this alarm. */
	tp = &(priv(rp)->s_alarm_timer);
	timerArg(tp)->ta_int = pNum;
	tp->tmr_func = causeAlarm;
	
	/* Return the ticks left on the previous alarm. */
	uptime = getUptime();
	if (tp->tmr_exp_time != TIMER_NEVER && uptime < tp->tmr_exp_time) 
	  msg->ALARM_TIME_LEFT = tp->tmr_exp_time - uptime;
	else
	  msg->ALARM_TIME_LEFT = 0;

	/* Finally, (re)set the timer depending on the expiration time. */
	if (expTime == 0) {
		resetTimer(tp);
	} else {
		tp->tmr_exp_time = useAbsTime ? expTime : expTime + getUptime();
		setTimer(tp, tp->tmr_exp_time, tp->tmr_func);
	}
	return OK;
}
