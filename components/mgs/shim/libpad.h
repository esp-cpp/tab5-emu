/* PSY-Q libpad: the controller API the game calls. The upstream ESP32 port
 * took this header from a private compatibility directory; these are the
 * declarations MGS actually uses (mts/mts_pad.c, game/gamed.c,
 * weapon/grenade.c), implemented by the Tab5 platform layer. */
#ifndef LIBPAD_H
#define LIBPAD_H

#ifdef __cplusplus
extern "C" {
#endif

#define PadStateDiscon 0
#define PadStateFindPad 1
#define PadStateFindCTP1 2
#define PadStateReqInfo 5
#define PadStateExecCmd 6
#define PadStateStable 7

/* PadInfoMode() / PadInfoAct() selectors */
#define InfoModeCurID 1
#define InfoModeCurExID 2
#define InfoModeCurExOffs 3
#define InfoModeIdTable 4
#define InfoActFunc 1
#define InfoActSub 2
#define InfoActSize 3
#define InfoActCurr 4
#define InfoActSign 5

void PadInitDirect(unsigned char *pad1, unsigned char *pad2);
void PadStartCom(void);
void PadStopCom(void);
int PadGetState(int port);
int PadInfoAct(int port, int actno, int term);
int PadSetActAlign(int port, char *data);
void PadSetAct(int port, unsigned char *data, int len);

#ifdef __cplusplus
}
#endif

#endif /* LIBPAD_H */
