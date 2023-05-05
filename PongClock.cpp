#include <Arduino.h>
#include "PongClock.h"
#include "RTC.h"
#include "Config.h"
#include "SimpleDLS.h"
#include "ILI948x.h"

// from my onHand pong clock
#if 0
// hack to temporarily free up program space
extern void pong_Init(){ }
extern void pong_Loop(){ }

#else
int ballSize = 15;
#define COLOUR  ColourByte
#define COLOUR_BACK  0x00
#define COLOUR_SCORE 0xFF
#define COLOUR_BALL  0xFF
#define COLOUR_NET   0xFF
#define COLOUR_LEFT  0xFF
#define COLOUR_RIGHT 0xFF

#define SCREEN_WIDTH    LCD_WIDTH
#define SCREEN_HEIGHT   LCD_HEIGHT
#define BALL_SIZE       ballSize
#define BALL_SPEED      (BALL_SIZE*3)
#define FONT_CELL_SIZE  ballSize
#define PADDLE_WIDTH    ballSize
#define PADDLE_HEIGHT   (5*ballSize)
#define SIDE_RIGHT      (1)
#define SIDE_LEFT       (0)
#define SIDE_NONE       (-1)
#define DWELL_COUNT     8
#define NET_WIDTH       2

int  ScoreTimeHH;
int  ScoreTimeMM;
int ballX, ballY,
ballPrevX, ballPrevY,
ballDeltaX, ballDeltaY;
int paddleLeftY, paddleRightY,   // of the centre   
paddleLeftPrevY, paddleRightPrevY;
int sideLosing, dwellCounter1 = 0, dwellCounter2 = 0;
unsigned long UpdateTimer;


void drawScoreDigit(int* x, int index)
{
    // draw the <index>th digit 0..9 (10 = blank)
    // at x, y. x is advanced to the next char

    // 15 bits, 5 rows of 3 cols, 15th bit is top left
    static word scoreFont[11] = { 0x7B6F, /*0x2492*/062227, 0x73E7, 0x73CF, 0x5BC9, 0x79CF, 0x79EF, 0x7249, 0x7BEF, 0x7BC9, 0x0000 };
    //                                              ^^^^^^ serif '1'

    word FontWord = scoreFont[index];
    word Mask = 0x4000;
    int cx, cy;

    for (cy = 0; cy < 5; cy++)
        for (cx = 0; cx < 3; cx++)
        {
            int sx = *x + cx * FONT_CELL_SIZE;
            int sy = cy * FONT_CELL_SIZE;

            if (FontWord & Mask)
                ILI948x::COLOUR(COLOUR_SCORE, ILI948x::Window(sx, sy, FONT_CELL_SIZE - 1, FONT_CELL_SIZE - 1));
            else
                ILI948x::COLOUR(COLOUR_BACK, ILI948x::Window(sx, sy, FONT_CELL_SIZE, FONT_CELL_SIZE));
            Mask >>= 1;
        }
    *x += 4 * FONT_CELL_SIZE;
}

void drawScoreTime(bool read = true)
{
    // the time is the score, 12 or 24-hour mode
    int x = SCREEN_WIDTH / 2 - 8 * FONT_CELL_SIZE;
    int hour;
    if (read)
    {
        rtc.ReadTime(true);
        ScoreTimeHH = SimpleDLS::GetDisplayHour24();//rtc.m_Hour24, rtc.m_DayOfMonth, rtc.m_Month, rtc.m_Year);
        ScoreTimeMM = rtc.m_Minute;
    }
    hour = ScoreTimeHH;
    if (config.mode24Hour)
    {
        drawScoreDigit(&x, hour / 10);
    }
    else
    {
        hour %= 12;
        if (hour == 0) hour = 12;
        if (hour < 10)
            drawScoreDigit(&x, 10); // blank
        else
            drawScoreDigit(&x, hour / 10);
    }
    drawScoreDigit(&x, hour % 10);
    x += FONT_CELL_SIZE + 1;    // over the net
    drawScoreDigit(&x, ScoreTimeMM / 10);
    drawScoreDigit(&x, ScoreTimeMM % 10);
}

void drawPaddles()
{
    // both sides
    int side;
    for (side = 0; side <= 1; side++)
    {
        int x = side ? SCREEN_WIDTH - PADDLE_WIDTH : 0;
        int y = side ? paddleRightY : paddleLeftY;
        int* prevY = side ? &paddleRightPrevY : &paddleLeftPrevY;
        y -= PADDLE_HEIGHT / 2 - BALL_SIZE / 2;
        if (y < 0)
            y = 0;
        if (y + PADDLE_HEIGHT > SCREEN_HEIGHT)
            y = SCREEN_HEIGHT - PADDLE_HEIGHT;
        if (*prevY != y)
        {
            // blank the old paddle
            if (*prevY != -1)
                ILI948x::COLOUR(COLOUR_BACK, ILI948x::Window(x, *prevY, PADDLE_WIDTH - 1, PADDLE_HEIGHT - 1));
            // draw the paddle
            ILI948x::COLOUR(side?COLOUR_LEFT:COLOUR_RIGHT, ILI948x::Window(x, y, PADDLE_WIDTH - 1, PADDLE_HEIGHT - 1));
            *prevY = y;
        }
    }
}

void movePaddles()
{
    // Align paddle with the ball if it's approaching...
    int sideApproaching = (ballDeltaX < 0) ? SIDE_LEFT : SIDE_RIGHT;
    int paddleY = ballY;
    if (sideLosing == sideApproaching)
    {
        // ... unless we need to miss. Just put it in the other half.
        paddleY -= SCREEN_HEIGHT / 2;
        if (paddleY < 0)
            paddleY = SCREEN_HEIGHT + paddleY;
    }

    if (sideApproaching == SIDE_LEFT)
        paddleLeftY = paddleY;
    else
        paddleRightY = paddleY;
}

void drawNet()
{
    long len = ILI948x::Window(SCREEN_WIDTH/2, 0, NET_WIDTH, SCREEN_HEIGHT - 1);
    while (len > 0)
    {
      ILI948x::COLOUR(COLOUR_NET, 4*NET_WIDTH);
      ILI948x::COLOUR(COLOUR_BACK, 4*NET_WIDTH);
      len -= 8*NET_WIDTH;
    }
}

void drawCourt()
{
    drawNet();
    drawScoreTime();
    drawPaddles();
}

int serveSign = +1;
int served = 0;
void serveBall()
{
    ballX = SCREEN_WIDTH / 2;
    // serve off the winning paddle
    if (sideLosing == SIDE_LEFT)
    {
        ballX = SCREEN_WIDTH - BALL_SIZE - PADDLE_WIDTH;
        serveSign = -1;
    }
    else if (sideLosing == SIDE_RIGHT)
    {
        ballX = PADDLE_WIDTH;
        serveSign = +1;
    }
    ballY = SCREEN_HEIGHT / 2 - 10 + random(20);
    ballDeltaX = serveSign * BALL_SPEED;
    ballDeltaY = BALL_SPEED / (2 + random(2));
    if (random(2))
        ballDeltaY = -ballDeltaY;
    serveSign = -serveSign;
    sideLosing = SIDE_NONE;
    served = 1;
}

void initPong()
{
    dwellCounter1 = dwellCounter2 = 0;
    serveBall();
    paddleLeftPrevY = paddleRightPrevY = ballPrevX = ballPrevY = -1;
    paddleLeftY = paddleRightY = ballY;
    drawCourt();
    drawPaddles();
}

void xorBall(int x, int y, bool on)
{
    // can't xor, fake it
    ILI948x::COLOUR(on?COLOUR_BALL:COLOUR_BACK, ILI948x::Window(x, y, BALL_SIZE, BALL_SIZE));
    if (!on)
    {
      // refresh the elements which may've been altered
      if (x <= SCREEN_WIDTH/2 && (x + BALL_SIZE) >= (SCREEN_WIDTH/2 + NET_WIDTH))
        drawNet();
      if (y < FONT_CELL_SIZE*5) // we don't we see leftovers between squares in the on cells?
        drawScoreTime(false);
    }
}

void updateBall()
{
    // main logic
    int bounceSide = SIDE_NONE;
    // handle delays in animation
    if (dwellCounter1)
    {
        dwellCounter1--;
        if (dwellCounter1 == 0)
        {
            drawScoreTime();
            dwellCounter2 = DWELL_COUNT;
        }
        return;
    }
    if (dwellCounter2)
    {
        dwellCounter2--;
        if (dwellCounter2 == 0)
        {
            serveBall();
        }
        return;
    }

    // move the ball (except on a serve, let it sit for one frame)
    if (!served)
    {
        ballX += ballDeltaX;
        ballY += ballDeltaY;
    }
    served = 0;
    // bounce left-right?
    if (ballX < PADDLE_WIDTH)
    {
        ballX = PADDLE_WIDTH;
        ballDeltaX = -ballDeltaX;
        bounceSide = SIDE_LEFT;
    }
    if (ballX >= (SCREEN_WIDTH - BALL_SIZE - PADDLE_WIDTH - 1))
    {
        ballX = SCREEN_WIDTH - BALL_SIZE - PADDLE_WIDTH - 1;
        ballDeltaX = -ballDeltaX;
        bounceSide = SIDE_RIGHT;
    }

    // bounce up-down?
    if (ballY <= 0)
    {
        ballY = 0;
        ballDeltaY = -ballDeltaY;
    }
    if (ballY >= (SCREEN_HEIGHT - BALL_SIZE - 1))
    {
        ballY = SCREEN_HEIGHT - BALL_SIZE - 1;
        ballDeltaY = -ballDeltaY;
    }

    if (ballPrevX != -1)    // undraw ball at previous location
        xorBall(ballPrevX, ballPrevY, false);
    ballPrevX = ballX;
    ballPrevY = ballY;
    xorBall(ballX, ballY, true);  // draw ball at new location
    if (bounceSide != SIDE_NONE)
    {
        if (bounceSide == sideLosing)
        {
            // bounced off the losing side, point!
            dwellCounter1 = DWELL_COUNT;  // linger showing loss and new score/time
        }
        else
        {
            // check if a side needs to lose
            rtc.ReadTime(true);
            int hour = SimpleDLS::GetDisplayHour24();//rtc.m_Hour24, rtc.m_DayOfMonth, rtc.m_Month, rtc.m_Year);
            if (hour != ScoreTimeHH)
                sideLosing = SIDE_RIGHT;
            else if (rtc.m_Minute != ScoreTimeMM)
                sideLosing = SIDE_LEFT;
        }
    }
    if (!dwellCounter1)
    {
        movePaddles();
        drawPaddles();
    }
}

extern void pong_Init()
{
    ILI948x::ColourByte(0, ILI948x::Window(0, 0, LCD_WIDTH, LCD_HEIGHT));
    ballPrevX = paddleRightPrevY = paddleLeftPrevY = -1;
    UpdateTimer = millis();
    drawCourt();
}

extern void pong_Loop()
{
    if (RTC::CheckPeriod(UpdateTimer, 100))
    {
      updateBall();
    }
}
#endif
