
#include "stm32f0xx.h"
#include <stdint.h>
#include <stdlib.h>
#include "lcd.h"

void nano_wait(unsigned int n) {
    asm(    "        mov r0,%0\n"
            "repeat: sub r0,#83\n"
            "        bgt repeat\n" : : "r"(n) : "r0", "cc");
}

void small_delay() {
    nano_wait(1000000); // wait one millisecond
}

void setup_tim17()
{
    // Enable the RCC clock for TIM17
    RCC -> APB2ENR |= RCC_APB2ENR_TIM17EN;
    // Set the prescaler and auto-reload register to result in a timer update event
    // exactly 100 times per second
    TIM17 -> PSC = 4800 - 1;
    TIM17 -> ARR = 100 - 1;
    // Enable the UIE bit in the DIER
    TIM17 -> DIER |= TIM_DIER_UIE;
    // Enable the timer
    TIM17 -> CR1 |= TIM_CR1_CEN;
    // Enable the Timer 17 interrupt in the NVIC ISER
    NVIC -> ISER[0] = (1 << TIM17_IRQn);
}

void setup_portb()
{
    // Enable Port B.
    RCC -> AHBENR |= RCC_AHBENR_GPIOBEN;
    // Set PB0-PB3 for output.
    GPIOB -> MODER &= ~(GPIO_MODER_MODER0 | GPIO_MODER_MODER1 | GPIO_MODER_MODER2 | GPIO_MODER_MODER3);
    GPIOB -> MODER |= (GPIO_MODER_MODER0_0 |GPIO_MODER_MODER1_0 | GPIO_MODER_MODER2_0 | GPIO_MODER_MODER3_0);
    // Set PB4-PB7 for input and enable pull-down resistors.
    GPIOB -> MODER &= ~(GPIO_MODER_MODER4 | GPIO_MODER_MODER5 | GPIO_MODER_MODER6 | GPIO_MODER_MODER7);
    GPIOB -> PUPDR &= ~(GPIO_PUPDR_PUPDR4 | GPIO_PUPDR_PUPDR5 | GPIO_PUPDR_PUPDR6 | GPIO_PUPDR_PUPDR7);
    GPIOB -> PUPDR |= (GPIO_PUPDR_PUPDR4_1 | GPIO_PUPDR_PUPDR5_1 | GPIO_PUPDR_PUPDR6_1 | GPIO_PUPDR_PUPDR7_1);
    // Turn on the output for the lower row (Row 4 -> PB3)
    GPIOB -> ODR |= GPIO_ODR_3;
}

// For 7 segment displays
void setup_portc()
{
    // Enable Port C
    RCC -> AHBENR |= RCC_AHBENR_GPIOCEN;
    // Configure pins PC0 – PC10 to be outputs
    GPIOC -> MODER &= ~(GPIO_MODER_MODER0 | GPIO_MODER_MODER1 | GPIO_MODER_MODER2
            | GPIO_MODER_MODER3 | GPIO_MODER_MODER4 | GPIO_MODER_MODER5 | GPIO_MODER_MODER6
            | GPIO_MODER_MODER7 | GPIO_MODER_MODER8 | GPIO_MODER_MODER9 | GPIO_MODER_MODER10);
    GPIOC -> MODER |= (GPIO_MODER_MODER0_0 | GPIO_MODER_MODER1_0 | GPIO_MODER_MODER2_0
            | GPIO_MODER_MODER3_0 | GPIO_MODER_MODER4_0 | GPIO_MODER_MODER5_0 | GPIO_MODER_MODER6_0
            | GPIO_MODER_MODER7_0 | GPIO_MODER_MODER8_0 | GPIO_MODER_MODER9_0 | GPIO_MODER_MODER10_0);
}

void set_display(char * display) {
    for (int off = 0; off < 8; off++) {
        GPIOC -> ODR = (off << 8) | get_display_code(display[off]);
        small_delay(); // wait one millisecond to let number display
    }
}

// Not as efficient as a dictionary/hashtable but should be fine?
int get_display_code(char letter) {
    if (letter == '1') {
        return 0x6;
    } else if (letter == '2') {
        return 0x5b;
    } else if (letter == '3') {
        return 0x4f;
    } else if (letter == '4') {
        return 0x66;
    } else if (letter == '5') {
        return 0x6d;
    } else if (letter == '6') {
        return 0x7d;
    } else if (letter == '7') {
        return 0x7;
    } else if (letter == '8') {
        return 0x7f;
    } else if (letter == '9') {
        return 0x67;
    } else if (letter == 'g') {
        return 0x6f;
    } else if (letter == 'a') {
        return 0x77;
    } else if (letter == 'm') {
        return 0x15;
    } else if (letter == 'e') {
        return 0x79;
    } else if (letter == 'o') {
        return 0x5c;
    } else if (letter == 'v') {
        return 0x1c;
    } else if (letter == 'r') {
        return 0x50;
    }
    return 0; // no matching character
}

char check_key()
{
    // If the '*' key is pressed, return '*'
    if (((GPIOB -> IDR) & (1 << 4)) > 0) {
        return '*';
    }
    // If the 'D' key is pressed, return 'D'
    if (((GPIOB -> IDR) & (1 << 7)) > 0) {
        return 'D';
    }
    // Otherwise, return 0
    return 0;
}

void setup_spi1()
{
    // Enable the RCC clock to GPIO Port A
    RCC -> AHBENR |= RCC_AHBENR_GPIOAEN;
    // Configure pins PA4, PA5, and PA7 for alternate function 0
    GPIOA -> MODER &= ~(GPIO_MODER_MODER4 | GPIO_MODER_MODER5 | GPIO_MODER_MODER7);
    GPIOA -> MODER |= (GPIO_MODER_MODER4_1 | GPIO_MODER_MODER5_1 | GPIO_MODER_MODER7_1);
    GPIOA -> AFR[0] &= ~(GPIO_AFRL_AFR4 | GPIO_AFRL_AFR5 | GPIO_AFRL_AFR7);
    // Configure pins PA2 and PA3 to be outputs
    GPIOA -> MODER &= ~(GPIO_MODER_MODER2 | GPIO_MODER_MODER3);
    GPIOA -> MODER |= (GPIO_MODER_MODER2_0 | GPIO_MODER_MODER3_0);
    // Enable the RCC clock to SPI1
    RCC -> APB2ENR |= RCC_APB2ENR_SPI1EN;
    // Configure SPI1 for Master/Bidimode/BidiOE, but set the baud rate as high as possible
    SPI1 -> CR1 |= (SPI_CR1_MSTR | SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE);
    SPI1 -> CR1 &= ~(SPI_CR1_BR);
    // Set SSOE/NSSP as you did for SPI2, but leave the word size set to 8-bit (the default)
    SPI1 -> CR2 = SPI_CR2_SSOE | SPI_CR2_NSSP;
    // Enable the SPI channel
    SPI1 -> CR1 |= SPI_CR1_SPE;
}

// Copy a subset of a large source picture into a smaller destination.
// sx,sy are the offset into the source picture.
void pic_subset(Picture *dst, const Picture *src, int sx, int sy)
{
    int dw = dst->width;
    int dh = dst->height;
    if (dw + sx > src->width)
        for(;;)
            ;
    if (dh + sy > src->height)
        for(;;)
            ;
    for(int y=0; y<dh; y++)
        for(int x=0; x<dw; x++)
            dst->pix2[dw * y + x] = src->pix2[src->width * (y+sy) + x + sx];
}

// Overlay a picture onto a destination picture.
// xoffset,yoffset are the offset into the destination picture that the
// source picture is placed.
// Any pixel in the source that is the 'transparent' color will not be
// copied.  This defines a color in the source that can be used as a
// transparent color.
void pic_overlay(Picture *dst, int xoffset, int yoffset, const Picture *src, int transparent)
{
    for(int y=0; y<src->height; y++) {
        int dy = y+yoffset;
        if (dy < 0 || dy >= dst->height)
            continue;
        for(int x=0; x<src->width; x++) {
            int dx = x+xoffset;
            if (dx < 0 || dx >= dst->width)
                continue;
            unsigned short int p = src->pix2[y*src->width + x];
            if (p != transparent)
                dst->pix2[dy*dst->width + dx] = p;
        }
    }
}

// Called after a bounce, update the x,y velocity components in a
// pseudo random way.  (+/- 1)
void perturb(int *vx, int *vy)
{
    if (*vx > 0) {
        *vx += (random()%3) - 1;
        if (*vx >= 3)
            *vx = 2;
        if (*vx == 0)
            *vx = 1;
    } else {
        *vx += (random()%3) - 1;
        if (*vx <= -3)
            *vx = -2;
        if (*vx == 0)
            *vx = -1;
    }
    if (*vy > 0) {
        *vy += (random()%3) - 1;
        if (*vy >= 3)
            *vy = 2;
        if (*vy == 0)
            *vy = 1;
    } else {
        *vy += (random()%3) - 1;
        if (*vy <= -3)
            *vy = -2;
        if (*vy == 0)
            *vy = -1;
    }
}

extern const Picture background; // A 240x320 background image
extern const Picture ball; // A 19x19 purple ball with white boundaries
extern const Picture paddle; // A 59x5 paddle

const int border = 20;
int xmin; // Farthest to the left the center of the ball can go
int xmax; // Farthest to the right the center of the ball can go
int ymin; // Farthest to the top the center of the ball can go
int ymax; // Farthest to the bottom the center of the ball can go
int x,y; // Center of ball
int vx,vy; // Velocity components of ball

int px; // Center of paddle offset
int newpx; // New center of paddle

char * display[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};

// This C macro will create an array of Picture elements.
// Really, you'll just use it as a pointer to a single Picture
// element with an internal pix2[] array large enough to hold
// an image of the specified size.
// BE CAREFUL HOW LARGE OF A PICTURE YOU TRY TO CREATE:
// A 100x100 picture uses 20000 bytes.  You have 32768 bytes of SRAM.
#define TempPicturePtr(name,width,height) Picture name[(width)*(height)/6+2] = { {width,height,2} }

// Create a 29x29 object to hold the ball plus padding
TempPicturePtr(object,29,29);

void TIM17_IRQHandler(void)
{
    TIM17->SR &= ~TIM_SR_UIF;
    char key = check_key();
    if (key == '*')
        newpx -= 1;
    else if (key == 'D')
        newpx += 1;
    if (newpx - paddle.width/2 <= border || newpx + paddle.width/2 >= 240-border)
        newpx = px;
    if (newpx != px) {
        px = newpx;
        // Create a temporary object two pixels wider than the paddle.
        // Copy the background into it.
        // Overlay the paddle image into the center.
        // Draw the result.
        //
        // As long as the paddle moves no more than one pixel to the left or right
        // it will clear the previous parts of the paddle from the screen.
        TempPicturePtr(tmp,61,5);
        pic_subset(tmp, &background, px-tmp->width/2, background.height-border-tmp->height); // Copy the background
        pic_overlay(tmp, 1, 0, &paddle, -1);
        LCD_DrawPicture(px-tmp->width/2, background.height-border-tmp->height, tmp);
    }
    x += vx;
    y += vy;
    if (x <= xmin) {
        // Ball hit the left wall.
        vx = - vx;
        if (x < xmin)
            x += vx;
        perturb(&vx,&vy);
    }
    if (x >= xmax) {
        // Ball hit the right wall.
        vx = -vx;
        if (x > xmax)
            x += vx;
        perturb(&vx,&vy);
    }
    if (y <= ymin) {
        // Ball hit the top wall.
        vy = - vy;
        if (y < ymin)
            y += vy;
        perturb(&vx,&vy);
    }
    if (y >= ymax - paddle.height &&
        x >= (px - paddle.width/2) &&
        x <= (px + paddle.width/2)) {
        // The ball has hit the paddle.  Bounce.
        int pmax = ymax - paddle.height;
        vy = -vy;
        if (y > pmax)
            y += vy;
    }
    else if (y >= ymax) {
        // The ball has hit the bottom wall.  Set velocity of ball to 0,0.
        vx = 0;
        vy = 0;
    }

    TempPicturePtr(tmp,29,29); // Create a temporary 29x29 image.
    pic_subset(tmp, &background, x-tmp->width/2, y-tmp->height/2); // Copy the background
    pic_overlay(tmp, 0,0, object, 0xffff); // Overlay the object
    pic_overlay(tmp, (px-paddle.width/2) - (x-tmp->width/2),
            (background.height-border-paddle.height) - (y-tmp->height/2),
            &paddle, 0xffff); // Draw the paddle into the image
    LCD_DrawPicture(x-tmp->width/2,y-tmp->height/2, tmp); // Re-draw it to the screen
    // The object has a 5-pixel border around it that holds the background
    // image.  As long as the object does not move more than 5 pixels (in any
    // direction) from it's previous location, it will clear the old object.
}

int main(void)
{
    setup_portb();
    setup_portc();
    setup_spi1();
    LCD_Init();

    // Draw the background.
    LCD_DrawPicture(0,0,&background);

    // Set all pixels in the object to white.
    for(int i=0; i<29*29; i++)
        object->pix2[i] = 0xffff;

    // Center the 19x19 ball into center of the 29x29 object.
    // Now, the 19x19 ball has 5-pixel white borders in all directions.
    pic_overlay(object,5,5,&ball,0xffff);

    // Initialize the game state.
    xmin = border + ball.width/2;
    xmax = background.width - border - ball.width/2;
    ymin = border + ball.width/2;
    ymax = background.height - border - ball.height/2;
    x = (xmin+xmax)/2; // Center of ball
    y = ymin;
    vx = 0; // Velocity components of ball
    vy = 1;

    px = -1; // Center of paddle offset (invalid initial value to force update)
    newpx = (xmax+xmin)/2; // New center of paddle

    setup_tim17();
}
