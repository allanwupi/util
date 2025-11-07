#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#define BUFFER_CHARS 80
#define DEFAULT_DIE_SIDES 20
#define DEFAULT_MAX_ROLLS 20
#define DEFAULT_MAX_HIST 20

const char *programName = "roll";
const char *programUsage = "usage: roll [-a] [-l | -m] [-h] dice ...";

enum LevelOfDetail {
	DEFAULT, // print the total, crits (nat 20s) and dice values
	LESS, // minimal: print only the total and crits (nat 20s)
	MORE, // verbose: print descriptor string, average, nat1s/nat20s, sum and modifier
};

struct ProgramSettings {
	enum LevelOfDetail detail;
	bool histogram; // print a histogram of the results
	bool seen_input;
	unsigned int max_rolls_to_print;
	unsigned int max_histogram_axis;
} settings = {DEFAULT, false, DEFAULT_MAX_ROLLS, DEFAULT_MAX_HIST};

struct DiceRoll {
	char descriptor[BUFFER_CHARS];
	unsigned int len;
	unsigned int sides;
	int rerolls; // positive for advantage, negative for disadvantage
	int modifier;
	long sum;
	double average;
	unsigned int data[];
};

int parse_flag(char flag);
struct DiceRoll *get_dice_roll(const char *arg);
void parse_roll(const char *arg, int *len, int *sides, int *rerolls, int *modifier);
unsigned int roll_die(unsigned int sides);
void calculate_dice_roll(struct DiceRoll *roll);
void print_roll_data(struct DiceRoll *roll);
void print_roll_format(struct DiceRoll *roll);

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "%s\n", programUsage);
		return EXIT_FAILURE;
	}
	long running_total = 0;
	srand(time(NULL));
	struct DiceRoll *roll = NULL;
	if (argv[0][strlen(argv[0])-1] == 'v')
		settings.detail = MORE;
	else if (argv[0][strlen(argv[0])-1] == 'h')
		settings.detail = MORE, settings.histogram = true;
	int num_flags = 0;
	for (int i = 1; i < argc; i++) {
		int j = 1;
		if (argv[i][0] == '-' && argv[i][j] != '\0') {
			while (argv[i][j] != '\0') {
				if (parse_flag(argv[i][j]) == EXIT_FAILURE) {
					fprintf(stderr, "%s: error: illegal option %c\n", programName, argv[i][j]);
					return EXIT_FAILURE;
				}
				j++;
				num_flags++;
			}
		} else {
			roll = get_dice_roll(argv[i]);
			if (roll == NULL) {
				fprintf(stderr, "%s: error: failed to parse arg %s\n", programName, argv[i]);
				return EXIT_FAILURE;
			}
			calculate_dice_roll(roll);
			print_roll_format(roll);
			running_total += roll->sum;
			free(roll);
			roll = NULL;
			if (!settings.seen_input) settings.seen_input = true;
		}
	}
	if (!settings.seen_input) {
		fprintf(stderr, "%s\n", programUsage);
		return EXIT_FAILURE;
	}
	if (settings.detail == MORE) printf("total = %ld\n", running_total);
	else if (argc-num_flags > 2) printf("%ld\n", running_total);
	return EXIT_SUCCESS;
}

int parse_flag(char flag) {
	switch (flag) {
		case 'a':
			settings.max_rolls_to_print = UINT_MAX;
			break;
		case 'l':
		case 'u':
			settings.detail = LESS;
			break;
		case 'm':
		case 'v':
			settings.detail = MORE;
			break;
		case 'h':
			settings.histogram = !settings.histogram;
			break;
		default:
			return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

struct DiceRoll *get_dice_roll(const char *arg) {
	struct DiceRoll *roll = NULL;
	int len = 0, sides = 0, rerolls = 0, modifier = 0;
	parse_roll(arg, &len, &sides, &rerolls, &modifier);
	if (len <= 0 || sides <= 0) return NULL; // failed arg parsing
	roll = (struct DiceRoll *) calloc(sizeof(struct DiceRoll) + len*sizeof(unsigned int), 1);
	if (roll == NULL) return NULL; // failed to allocate memory
	int buffered = 0;
	if (modifier) buffered += snprintf(roll->descriptor, BUFFER_CHARS, "%id%i%+i", len, sides, modifier);
	else buffered += snprintf(roll->descriptor, BUFFER_CHARS, "%id%i", len, sides);
	if (rerolls != 0) {
		if (abs(rerolls) > 1) buffered += snprintf(roll->descriptor+buffered, BUFFER_CHARS-buffered, " with x%i ", abs(rerolls));
		else buffered += snprintf(roll->descriptor+buffered, BUFFER_CHARS-buffered, " with ");
		if (rerolls < 0) buffered += snprintf(roll->descriptor+buffered, BUFFER_CHARS-buffered, "dis");
		snprintf(roll->descriptor+buffered, BUFFER_CHARS-buffered, "advantage");
	}
	roll->len = len;
	roll->sides = sides;
	roll->rerolls = rerolls;
	roll->modifier = modifier;
	return roll;
}

void parse_roll(const char *arg, int *len, int *sides, int *rerolls, int *modifier) {
	char dest[BUFFER_CHARS] = {0};
	strncpy(dest, arg, BUFFER_CHARS-1);
	int i = 0;
	int sign = 1;
	char *len_str = dest;
	char *sides_str = NULL;
	char *rerolls_str = NULL;
	char *modifier_str = NULL;
	while (dest[i] != '\0') {
		if (dest[i] == 'd') {
			dest[i] = '\0';
			sides_str = dest+i+1;
		} else if (dest[i] == '^') {
			dest[i] = '\0';
			rerolls_str = dest+i+1;
		} else if (dest[i] == '+') {
			sign = +1;
			dest[i] = '\0';
			modifier_str = dest+i+1;
		} else if (dest[i] == '-') {
			if (rerolls_str != dest+i) {
				sign = -1;
				dest[i] = '\0';
				modifier_str = dest+i+1;
			}
		}
		i++;
	}
	if (len_str[0]) *len = atoi(len_str);
	else *len = 1; // roll 1 die by default
	if (sides_str != NULL && sides_str[0]) *sides = atoi(sides_str);
	else *sides = DEFAULT_DIE_SIDES; // set to 20 by default
	if (rerolls_str != NULL && rerolls_str[0]) *rerolls = atoi(rerolls_str);
	else *rerolls = 0;
	if (modifier_str != NULL && modifier_str[0]) *modifier = sign * atoi(modifier_str);
	else *modifier = 0;
}

unsigned int roll_die(unsigned int sides) {
	const int LIMIT = RAND_MAX - (RAND_MAX % sides);
	int r = RAND_MAX;
	while (r >= LIMIT) r = rand();
	return r % sides +1;
}

void calculate_dice_roll(struct DiceRoll *roll) {
	unsigned int curr_roll = 0;
	unsigned int reroll = 0;
	for (int i = 0; i < roll->len; i++) {
		curr_roll = roll_die(roll->sides);
		if (roll->rerolls != 0) {
			int num_rerolls = abs(roll->rerolls);
			for (int j = 0; j < num_rerolls; j++) {
				reroll = roll_die(roll->sides);
				if (roll->rerolls > 0)
					curr_roll = (reroll > curr_roll) ? reroll : curr_roll;
				else
					curr_roll = (reroll < curr_roll) ? reroll : curr_roll;
			}
		}
		roll->data[i] = curr_roll;
		roll->sum += curr_roll;
	}
	roll->average = (double)roll->sum / roll->len;
	roll->sum += roll->modifier;
}

void print_roll_data(struct DiceRoll *roll) {
	for (int i = 0; i < roll->len; i++) {
		if (i < settings.max_rolls_to_print) printf("%i",roll->data[i]);
		if (i == settings.max_rolls_to_print) {
			printf("..."); 
			break;
		}
		if (i < roll->len-1) printf(",");
	}
}

void print_roll_format(struct DiceRoll *roll) {
	unsigned int *hist = calloc(roll->sides+1, sizeof(unsigned int));
	for (int i = 0; i < roll->len; i++) {
		hist[roll->data[i]]++;
	}
	if (settings.detail == DEFAULT || settings.detail == LESS) {
		printf("%ld", roll->sum);
		if (roll->sides == 20 && hist[20] > 0) {
			if (hist[20] == 1) printf(" (+1 crit)");
			else printf(" (+%d crits)", hist[20]);
		}
		if (settings.detail != LESS) {
			printf("\n    : ");
			print_roll_data(roll);
		}
		printf("\n");
	}
	if (settings.detail == MORE) {
		printf("rolled %s:\n    data = [", roll->descriptor);
		print_roll_data(roll);
		printf("]\n    avg = %.3f\n", roll->average);
		if (roll->sides == 20 && hist[1])  printf("    nat1 = %d\n", hist[1]);
		if (roll->sides == 20 && hist[20]) printf("    nat20 = %d\n", hist[20]);
		printf("    sum = %ld (%s%d)\n", roll->sum - roll->modifier, (roll->modifier >= 0) ? "+" : "", roll->modifier);
	}
	if (settings.histogram) {
		const char *BAR = "##################################################";
		const int BAR_LEN = 50;
		int MAX = hist[1];
		for (int i = 2; i <= roll->sides; i++) MAX = (MAX < hist[i]) ? hist[i] : MAX;
		int width = 1;
		while (pow(10, width) <= MAX) width++;
		int axis_width = 1;
		while (pow(10, axis_width) <= roll->sides) axis_width++;
		printf("    hist = [\n");
		for (int i = 1; i <= roll->sides; i++) {
			if (hist[i] != 0 || roll->sides <= settings.max_histogram_axis)
				printf("        %*dx %*d: %.*s\n", width, hist[i], axis_width, i, (BAR_LEN*hist[i]+MAX/2)/MAX, BAR);
			if (i == roll->sides) printf("    ]\n");
		}
	}
	free(hist);
}