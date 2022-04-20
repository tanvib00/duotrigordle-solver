#include <set>
#include <string>
#include <iostream>
#include <fstream>
#include <iterator>
#include <chrono>

using namespace std;

#define MAX_GUESSES 15
//enum Guess { black, yellow, green };
// OR we could just do
#define BLACK 0
#define YELLOW 1
#define GREEN 2

template<typename S>
auto select_random(const S &s, size_t n) {
    auto it = std::begin(s);
    // 'advance' the iterator n times
    std::advance(it,n);
    return it;
}

int main() {

    /* Set up the game */
    // read in word data set
    set<string> dataset;

    char filename[] = "../words.dat.txt";
    fopen(filename, "r");
    string line;
    ifstream input_file(filename);
    if (!input_file.is_open()) {
        cerr << "Could not open the file - '"
             << filename << "'" << endl;
        return -1;
    }

    while (getline(input_file, line)){
        dataset.insert(line);
    }

    // goalWord = select random word from data set
    chrono::microseconds us = chrono::duration_cast< chrono::microseconds >(
    chrono::system_clock::now().time_since_epoch());
    srand(static_cast<unsigned int>(us.count()));
    auto r = rand() % dataset.size();
    string goalWord = *select_random(dataset, r);

    cout << "GOAL WORD: " << goalWord << endl;

    // TODO make smarter initial guess? 'adieu'

    bool solved = false;
    uint numGuesses = 0;
    while (solved == false && numGuesses < MAX_GUESSES) {
        // for now, just select random guess from list of possible next guesses?
        // eventually, figure out how to make the guess more smart
        // cout << "GOAL WORD: " << goalWord << endl;
        cout << "Dataset size: " << dataset.size() << endl;

        us = chrono::duration_cast< chrono::microseconds >(
        chrono::system_clock::now().time_since_epoch());
        srand(static_cast<unsigned int>(us.count()));
        auto r = rand() % dataset.size();
        string guess = *select_random(dataset, r);
        cout << "Guess " << numGuesses << ": " << guess << endl;

        // get feedback from game (compare guess to goalWord)
        uint correct = 0;
        int wordResult[5] = {BLACK, BLACK, BLACK, BLACK, BLACK}; // each has value of either 0,1,2 :: black,yellow,green
        bool goalLetterMatches[5] = {false, false, false, false, false};

        // check for greens first
        for (auto i = 0; i < guess.length(); i++) {
            char c = guess[i];
            if (c == goalWord[i]) {
                correct++;
                // update tuple with value 2 at index i
                goalLetterMatches[i] = true;
                wordResult[i] = GREEN;
            }
        }

        if (correct == 5) {
            solved = true;
            break;
        }

        // check for yellows
        for (auto i = 0; i < guess.length(); i++) {
            if (wordResult[i] == GREEN) continue;

            char c = guess[i];
            for (auto j = 0; j < goalWord.length(); j++) {
                if (c == goalWord[j] && goalLetterMatches[j] == false) {
                    goalLetterMatches[j] = true;
                    // update tuple with value 1 at index i
                    wordResult[i] = YELLOW;
                    break;
                }
            }
        }

        cout << "wordResult: [ ";
        for (int i = 0; i < 5; i++) {
            cout << wordResult[i] << " ";
        }
        cout << "]" << endl;

        // reduce list of possible guesses based on current information (wordResult)
        auto word_ptr = dataset.begin();
        while (word_ptr != dataset.end()) {
            string word = *word_ptr;
            bool wordLetterMatches[5] = {false, false, false, false, false}; // refers to letters in 'word' being matched to feedback from guess
            bool wordValid = true;

            // cout << "Word: " << word << " ... ";

            // check greens first
            for (int idx = 0; idx < guess.length(); idx++) {
                char guessLetter = guess[idx];
                int letterResult = wordResult[idx];
                // check word for green letters in correct positions
                if (letterResult == GREEN) {
                    if (word[idx] == guessLetter) {
                        wordLetterMatches[idx] = true;
                    }
                    else {
                        // letter in word does not match the letter from the guess at this idx
                        // remove from possible set of words
                        // cout << "ERASED G: " << guessLetter << endl;
                        word_ptr = dataset.erase(word_ptr);
                        //num_revoved++;
                        wordValid = false;
                        break;
                    }
                }
            }
            if (!wordValid) continue;

            // check yellows
            for (int idx = 0; idx < guess.length(); idx++) {
                char guessLetter = guess[idx];
                int letterResult = wordResult[idx];
                // check for yellow letters in word and not green
                if (letterResult == YELLOW) {
                    // search word for guessLetter
                    // goal: stall, guess: hello, word: swila, result: {b, b, y, g, b}
                    // TODO: can't be a match if the yellow matching character should have been green
                        // meaning if the yellow match is at the same index, this isn't good since we know it's not green

                    if (guessLetter == word[idx]) {
                        word_ptr = dataset.erase(word_ptr);
                        wordValid = false;
                        break;
                    }

                    bool letterFound = false;
                    for (int i = 0; i < word.length(); i++) {
                        if (guessLetter == word[i] && wordLetterMatches[i] == false) {
                            wordLetterMatches[i] = true;
                            letterFound = true;
                            break;
                        }
                    }
                    if (!letterFound) {
                        // cout << "ERASED Y: " << guessLetter << endl;
                        word_ptr = dataset.erase(word_ptr);
                        wordValid = false;
                        break;
                    }
                }
            }
            if (!wordValid) continue;

            // check blacks
            for (int idx = 0; idx < guess.length(); idx++) {
                char guessLetter = guess[idx];
                int letterResult = wordResult[idx];
                // check for not containing black letters
                if (letterResult == BLACK) {
                    // search word and make sure does not contain letter
                    // be careful for duplicates
                    // goal: frale, guess: stlal, word: plale, result: {b, b, y, y, b}
                    // guess: llama, word: while, {}
                    for (int i = 0; i < word.length(); i++) {
                        if (guessLetter == word[i] && wordLetterMatches[i] == false) {
                            // not valid
                            // cout << "ERASED B: " << guessLetter << endl;
                            word_ptr = dataset.erase(word_ptr);
                            wordValid = false;
                            break;
                        }
                    }
                    if (!wordValid) break;
                }
            }

            if (wordValid) {
                // cout << "Valid" << endl;
                word_ptr++;
            }
        } // end of candidate word for loop
        numGuesses++;
    }

    cout << "Solved: " << solved << endl;
    return numGuesses;
}
