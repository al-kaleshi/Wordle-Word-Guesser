#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> // for time()

#define WORD_LEN 5
#define ALPHABET_LEN 26
#define NUM_VOWELS 5
#define FILE_NAME "wordsLarge.txt"
//#define FILE_NAME "wordsTiny.txt"

struct Word {
  char value[WORD_LEN + 1];
  int score;
};
typedef struct Word Word;

struct Wordle {
  Word *words;
  size_t size;
  size_t capacity;
};
typedef struct Wordle Wordle;

struct Match {
  char exact[WORD_LEN];
  struct Partial {
    bool value;
    bool exclude[WORD_LEN];
  } partial[ALPHABET_LEN];
};
typedef struct Match Match;

int compare(const void *a, const void *b) {
  int firstScore = ((Word *)a)->score;
  int secondScore = ((Word *)b)->score;

  if (firstScore != secondScore) {
    return secondScore - firstScore;
  } else {
    return strcmp(((Word *)b)->value, ((Word *)a)->value);
  }
}

void copyString(char src[], char dest[], int size) {
  for (int idx = 0; idx < size; idx++) {
    dest[idx] = src[idx];
  }
  dest[size] = '\0';
}

bool stringContainsLetter(char src[], int size, char letter) {
  int contains = false;

  for (int idx = 0; idx < size; idx++) {
    if (src[idx] == letter) {
      contains = true;
      break;
    }
  }

  return contains;
}

bool stringEqual(char str1[], char str2[], int size) {
  bool match = true;
  for (int idx = 0; idx < size; idx++) {
    if (str1[idx] != str2[idx]) {
      match = false;
      break;
    }
  }

  return match;
}

Match createMatch() {
  Match match;

  for (int idx = 0; idx < WORD_LEN; idx++) {
    match.exact[idx] = '?';
  }

  for (int idx = 0; idx < ALPHABET_LEN; idx++) {
    match.partial[idx].value = false;

    for (int i = 0; i < WORD_LEN; i++) {
      match.partial[idx].exclude[i] = false;
    }
  }

  return match;
}

Wordle createWordle() {
  Word *words = (Word *)malloc(sizeof(Word) * 2);
  return (Wordle){.words = words, .capacity = 2, .size = 0};
}

void destroyWordle(Wordle *wordle) {
  free(wordle->words);
  wordle->capacity = 0;
  wordle->size = 0;
}

void addWordToWordle(Wordle *wordle, Word word) {
  if (wordle->capacity == wordle->size) {
    Word *words = (Word *)malloc(sizeof(Wordle) * wordle->capacity * 2);

    for (int idx = 0; idx < wordle->capacity; idx++) {
      words[idx] = wordle->words[idx];
    }

    wordle->words = words;
    wordle->capacity *= 2;
  }

  wordle->words[wordle->size] = word;
  wordle->size += 1;
}

bool isVowel(char letter) {
  bool match = false;
  char vowels[NUM_VOWELS] = {'a', 'e', 'i', 'o', 'u'};

  for (int idx = 0; idx < NUM_VOWELS; idx++) {
    if (vowels[idx] == letter) {
      match = true;
      break;
    }
  }

  return match;
}

int countUniqueVowels(char str[], int size) {
  int count = 0;
  char seen[NUM_VOWELS] = {'\0', '\0', '\0', '\0', '\0'};
  int seenIdx = 0;

  for (int idx = 0; idx < size; idx++) {
    char letter = str[idx];

    if (isVowel(letter)) {
      bool found = false;
      for (int i = 0; i < seenIdx; i++) {
        if (seen[i] == letter) {
          found = true;
          break;
        }
      }

      if (!found) {
        seen[seenIdx] = letter;
        seenIdx++;
        count++;
      }
    }
  }

  return count;
}

Word *getInitialWord(Wordle wordle) {
  int maxVowelCount = countUniqueVowels(wordle.words[0].value, WORD_LEN);
  Word *word = &wordle.words[0];
  for (int idx = 0; idx < wordle.size; idx++) {
    int vowelCount = countUniqueVowels(wordle.words[idx].value, WORD_LEN);

    if (vowelCount > maxVowelCount) {
      maxVowelCount = vowelCount;
      word = &wordle.words[idx];
    }
  }

  return word;
}

Wordle parseWordleFromFile() {
  Wordle wordle = createWordle();
  FILE *file = fopen(FILE_NAME, "r");
  char *line = NULL;
  size_t len = 0;

  while (getline(&line, &len, file) != -1) {
    Word word = {.score = 0};

    copyString(line, word.value, WORD_LEN);
    word.value[WORD_LEN] = '\0';
    addWordToWordle(&wordle, word);
  }
  free(line);
  fclose(file);

  return wordle;
}

void resetScore(Wordle wordle) {
  for (int idx = 0; idx < wordle.size; idx++) {
    wordle.words[idx].score = 0;
  }
}

void updateMatch(Match *match, Word word, char secret[]) {
  for (int idx = 0; idx < WORD_LEN; idx++) {
    if (word.value[idx] == secret[idx]) {
      match->exact[idx] = secret[idx];
    }
  }

  for (int idx = 0; idx < WORD_LEN; idx++) {
    if (stringContainsLetter(secret, WORD_LEN, word.value[idx])) {

      match->partial[word.value[idx] - 'a'].value = true;
      match->partial[word.value[idx] - 'a'].exclude[idx] = true;
    }
  }
}

void updateScore(Wordle *wordle, Match match) {
  for (int idx = 0; idx < wordle->size; idx++) {
    if (wordle->words[idx].score == -1) {
      continue;
    }

    int score = 0;
    bool partialSeen[ALPHABET_LEN];

    for (int i = 0; i < ALPHABET_LEN; i++) {
      partialSeen[i] = false;
    }

    for (int i = 0; i < WORD_LEN; i++) {

      if (match.exact[i] != '?' &&
          match.exact[i] != wordle->words[idx].value[i]) {
        score = -1;
        break;
      }

      if (match.exact[i] == wordle->words[idx].value[i]) {
        score += 2;
        continue;
      }

      char letter = wordle->words[idx].value[i];
      if (match.partial[letter - 'a'].value &&
          !match.partial[letter - 'a'].exclude[i] &&
          !partialSeen[letter - 'a']) {
        score += 1;
        partialSeen[letter - 'a'] = true;
        continue;
      }

      if (match.partial[letter - 'a'].value &&
          match.partial[letter - 'a'].exclude[i]) {
        score = -1;
        break;
      }
    }

    wordle->words[idx].score = score;
  }
}
void printGuess(int attempts, Word word, char secret[]) {
  char guess[WORD_LEN] = {'\0', '\0', '\0', '\0', '\0'};
  char hints[WORD_LEN] = {' ', ' ', ' ', ' ', ' '};
  Match match = createMatch();
  updateMatch(&match, word, secret);
  for (int idx = 0; idx < WORD_LEN; idx++) {
    if (match.exact[idx] != '?') {
      guess[idx] = toupper(word.value[idx]);
      continue;
    }
    if (match.exact[idx] == '?') {
      guess[idx] = word.value[idx];
    }
    char letter = word.value[idx];
    if (match.partial[letter - 'a'].value) {
      hints[idx] = '*';
    }
  }
  printf("%5d. ", attempts + 1);
  for (int idx = 0; idx < WORD_LEN; idx++) {
    printf("%c ", guess[idx]);
  }
  printf("\n");
  for (int idx = 0; idx < WORD_LEN; idx++) {
    if (idx == 0) {
      printf("       %c ", hints[idx]);
    } else {
      printf("%c ", hints[idx]);
    }
  }
  printf("\n");
}
void printSecretWordList(Wordle *wordle, char secret[]) {
  Match match = createMatch();
  Word *word = getInitialWord(*wordle);
  int attempts = 0;

  printf("Trying to find secret word: \n");
  // Display secret word with a space between letters, to match the guess words
  // below.
  printf("       ");
  for (int i = 0; i < WORD_LEN; i++) {
    printf("%c ", secret[i]);
  }
  printf("\n");
  printf("\n");

  while (!stringEqual(word->value, secret, WORD_LEN)) {
    updateMatch(&match, *word, secret);
    updateScore(wordle, match);
    word->score = -1;

    printGuess(attempts, *word, secret);

    // printf("WORD: %s, SECRET: %s\n", word->value, secret);
    // printMatch(match);
    // printWordle(*wordle);
    qsort(wordle->words, wordle->size, sizeof(Word), compare);
    word = &wordle->words[0];

    if (word->score == -1) {
      printf("NOT FOUND\n");
      break;
    }
    attempts++;
  }
  printGuess(attempts, *word, secret);
  printf("\nGOT IT!\n");
}

int main(void) {
  Wordle wordle = parseWordleFromFile();
  char userInput[81];
  srand((unsigned)time(NULL));
  printf("Using file %s with %ld words. \n", FILE_NAME, wordle.size);
  for (int i = 0; i < 3; i++) {
    printf("-----------------------------------------------------------\n");
    printf("\n");
    printf("Enter a secret word or just r to choose one at random: ");
    scanf(" %s", userInput);
    int length = (int)strlen(userInput);
    if (userInput[length] == '\n') {
      userInput[length] = '\0';
    }
    if (strlen(userInput) <= 1) {
      // Randomly select a secret word to be guessed.

      int randomIndex = rand() % wordle.size;
      strcpy(userInput, wordle.words[randomIndex].value);
    }
    printSecretWordList(&wordle, userInput);
    resetScore(wordle);
  }
  destroyWordle(&wordle);
  return EXIT_SUCCESS;
}
