#ifndef _CONTROL_H_
#define _CONTROL_H_

int control_init(const char* ip);
void control_poll_event(void);
void control_term(void);

#endif
