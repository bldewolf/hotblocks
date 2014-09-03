#include<string.h>
#include <stdio.h>
#include<curses.h>
#include<stdlib.h>
#include  <time.h>


/* gcc -o hotblocks hotblocks.c -std=gnu99 -lncurses -Wall */

#define BOARDH 22
#define BOARDW 10
struct game {
	int x, y, d, b, c;
	char board[BOARDH][BOARDW + 1]; // +1 for nulls
};

int blocks[][4][2] = {
	// pipe
	{ {-1, 0}, { 0, 0}, { 1, 0}, { 2, 0} },
	// left L
	{ {-1, 0}, { 0, 0}, { 1, 0},
	  { 1, 1} },
	// right L
	{ {-1, 0}, { 0, 0}, { 1, 0},
	                    {-1, 1} },
	// T
	{ {-1, 0}, { 0, 0}, { 1, 0},
	           { 0, 1} },
	// left S
	{ {-1, 0}, { 0, 0},
	           { 0, 1}, { 1, 1} },
	// right S
	{          { 0, 0}, { 1, 0},
	  {-1, 1}, { 0, 1} },
	// block
	{ { 0, 0}, { 1, 0},
	  { 0, 1}, { 1, 1} },
	};

int blocks_len = sizeof(blocks) / sizeof(blocks[0]);


void setup_board(struct game *g) {
	memset(g->board, ' ', sizeof(g->board));
	for(int i = 0; i < BOARDH; i++)
		g->board[i][BOARDW] = 0;
}

/* directions are counter clockwise, but since we're displaying things upside
 * down... */
void transform(int *x, int *y, int dir) {
	int t;
	switch(dir) {
		case 0:
			break;
		case 1:
			t = *x;
			*x = -*y;
			*y = t;
			break;
		case 2:
			*x = -*x;
			*y = -*y;
			break;
		case 3:
			t = *x;
			*x = *y;
			*y = -t;
			break;
	}
}

int test_piece(struct game *g, int x, int y, int dir, int block) {
	for(int i = 0; i < 4; i++) {
		int dx = blocks[block][i][0];
		int dy = blocks[block][i][1];

		transform(&dx, &dy, dir);

		if(y + dy < 0 || y + dy >= BOARDH ||
		   x + dx < 0 || x + dx >= BOARDW ||
		   g->board[y + dy][x + dx] != ' ')
			return 0;
	}
	return 1;
}

int test_player(struct game *g) {
	return test_piece(g, g->x, g->y, g->d, g->b);
}


int new_piece(struct game *g) {
	g->x = 5;
	g->y = 1;
	g->d = 0;
	g->b = rand() % blocks_len;
	g->c = (rand() % 26) + 'A';
	return test_player(g);
}
	

void set_piece(struct game *g, int x, int y, int dir, int block, char color) {
	for(int i = 0; i < 4; i++) {
		int dx = blocks[block][i][0];
		int dy = blocks[block][i][1];

		transform(&dx, &dy, dir);

		g->board[y + dy][x + dx] = color;
	}
}

void set_player(struct game *g) {
	set_piece(g, g->x, g->y, g->d, g->b, g->c);
}

void clear_player(struct game *g) {
	set_piece(g, g->x, g->y, g->d, g->b, ' ');
}

int move_player(struct game *g, int dx, int dy, int dd) {
	if(test_piece(g, g->x + dx, g->y + dy, (g->d + dd) % 4, g->b)) {
		g->x += dx;
		g->y += dy;
		g->d = (g->d + dd) % 4;
		return 1;
	}
	return 0;
}

void draw_board(WINDOW *win, struct game *g) {
	set_player(g);
	for(int i = 2; i < BOARDH; i++)
		mvwaddstr(win, i-1, 1, g->board[i]);
	clear_player(g);
	wrefresh(win);
	move(0, 0);
}

int get_msec() {
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

int eat_lines(struct game *g) {
	int sum = 0;
	for(int i = BOARDH - 1; i >= 0; i--) {
		if(!strchr(g->board[i], ' ')) {
			if(i > 0) {
				memmove(g->board[1],
					g->board[0],
					sizeof(g->board[0]) * i);
				i++;
			}
			memset(g->board, ' ', sizeof(g->board[0]));
			g->board[0][BOARDW] = 0;

			sum += 1;
		}
	}

	return sum;
}

int main(int argc, char *argv[]) {
	char c;
	WINDOW *win;
	struct game g;
	int last_msec, freq = 500;
	int score = 0;

	srand(time(0)); // I am a bad person

	initscr();
	refresh();
	noecho();
	cbreak();

	// Window includes the border lines
	win = newwin(BOARDH, BOARDW + 2, 0, 0);
	box(win, 0, 0);
	wrefresh(win);

	setup_board(&g);
	new_piece(&g);

	last_msec = get_msec();
	draw_board(win, &g);
	timeout(50);
	while((c = getch())) {
		int ret = 0;
		int now = get_msec();
		switch(c) {
			case 'a':
				ret = move_player(&g, -1, 0, 0);
				break;
			case 's':
				ret = move_player(&g, 0, 1, 0);
				if(ret)
					last_msec = now;
				break;
			case 'd':
				ret = move_player(&g, 1, 0, 0);
				break;
			case 'w':
				ret = move_player(&g, 0, -1, 0);
				break;
			case 'f':
				ret = move_player(&g, 0, 0, 1);
				break;
			case ' ':
				last_msec = now - freq - 1;
				while(move_player(&g, 0, 1, 0)) {
					last_msec = now;
				}
				break;
			case 'q':
				delwin(win);
				endwin();
				printf("Score: %d\n", score);
				exit(0);
			default:
				break;
		}
		draw_board(win, &g);
		if(now - last_msec > freq) {
			last_msec = now;
			if(!move_player(&g, 0, 1, 0)) {
				set_player(&g);
				score += eat_lines(&g);
				if(!new_piece(&g)) {
					delwin(win);
					endwin();
					printf("Score: %d\n", score);
					exit(0);
				}
			}
		}
	}

	return 0;
}
