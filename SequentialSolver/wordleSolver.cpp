#include <set>
#include <string>
#include <iostream>
#include <fstream>
#include <iterator>
#include <chrono>
#include <vector>

using namespace std;

#define MAX_GUESSES 40
#define NWORDLES 32
//enum Guess { black, yellow, green };
// OR we could just do
#define BLACK 0
#define YELLOW 1
#define GREEN 2

set<string> dictionary;
// for 32 games, 32 dif datasets, 32 goal words
set<string> datasets[NWORDLES];
vector<string> goal_words;
vector<bool> solved;
int solved_cnt;

template<typename S>
auto select_random(const S &s, size_t n) {
    auto it = std::begin(s);
    // 'advance' the iterator n times
    std::advance(it,n);
    return it;
}

/* Set up the game */
void init_data() {
    // read in word data set
    char filename[] = "../words.dat.txt";
    fopen(filename, "r");
    string line;
    ifstream input_file(filename);
    if (!input_file.is_open()) {
        cerr << "Could not open the file - '"
             << filename << "'" << endl;
        return;
    }

    while (getline(input_file, line)){
        dictionary.insert(line);
    }
}

int *get_feedback(int idx, string goalWord, string guess) {
    // get feedback from game (compare guess to goalWord)
    uint correct = 0;
    int *wordResult = new int[5];
    for (auto j=0; j<5; j++) {
        wordResult[j] = BLACK; // each has value of either 0,1,2 :: black,yellow,green
    }
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
        solved_cnt++;
        solved[idx] = true;
        return wordResult;
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

    return wordResult;
}

void reduce_dataset(int wnum, string guess, int *wordResult) {
    /*cout << "wnum " << i << ", goal: " << goal_words[i] << ", guess: " << guess << ", wordResult: [ ";
    for (int j = 0; j < 5; j++) {
        cout << wordResult[j] << " ";
    }
    cout << "]" << endl;*/
    // reduce list of possible guesses based on current information (wordResult)
    auto word_ptr = datasets[wnum].begin();
    while (word_ptr != datasets[wnum].end()) {
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
                    //cout << "ERASED G: " << guessLetter << endl;
                    word_ptr = datasets[wnum].erase(word_ptr);
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
                    //cout << "ERASED Y2: " << guessLetter << endl;
                    word_ptr = datasets[wnum].erase(word_ptr);
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
                    //cout << "ERASED Y: " << guessLetter << endl;
                    word_ptr = datasets[wnum].erase(word_ptr);
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
                        //cout << "ERASED B: " << guessLetter << endl;
                        word_ptr = datasets[wnum].erase(word_ptr);
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
}

string select_guess() {
    // check all datasets and pick one
    // for now, just selects first guess from listS of possible next guesses
    string all_guesses[NWORDLES];

    int idx_to_guess;

    for (auto i=NWORDLES-1; i>-1; i--) {
        if (solved[i]) continue;
        else idx_to_guess = i;
        chrono::microseconds us = chrono::duration_cast< chrono::microseconds >(
        chrono::system_clock::now().time_since_epoch());
        srand(static_cast<unsigned int>(us.count()));
        auto r = rand() % datasets[i].size();
        string guess = *select_random(datasets[i], r);
        all_guesses[i] = guess;
        //cout << "dsize " << i << ": " << datasets[i].size() << ", ";
    }
    //cout << endl;
    return all_guesses[idx_to_guess]; // currently just returns the first unsolved wordle's guess
                           // TODO make this better ?
}

int main() {
    init_data();

    // goal words = select randomly from data set
    for (auto i=0; i<NWORDLES; i++) {
        chrono::microseconds us = chrono::duration_cast< chrono::microseconds >(
        chrono::system_clock::now().time_since_epoch());
        srand(static_cast<unsigned int>(us.count()));
        auto r = rand() % dictionary.size();
        string goalWord = *select_random(dictionary, r);
        goal_words.push_back(goalWord);
        solved.push_back(false); // initialize all to unsolved

        copy( dictionary.begin(), dictionary.end(), inserter(datasets[i], datasets[i].begin()) ); // copy dict to each dataset

        cout << "GOAL WORD " << i << ": " << goalWord << endl;
    }

    solved_cnt = 0;
    bool all_solved = false;
    uint num_guesses = 0; // dont technically need an array of these, bc one guess applies to all boards
    while (!all_solved && num_guesses < MAX_GUESSES) {
        string guess = select_guess();
        num_guesses++;
        cout << "guess " << num_guesses << ": " << guess << endl;
        for (auto i=0; i<NWORDLES; i++) {
            if (solved[i]) continue; // don't re-check word if already solved
            int *wordResult = get_feedback(i, goal_words[i], guess);
            if (solved_cnt == NWORDLES) {
                all_solved = true;
                break;
            }
            reduce_dataset(i, guess, wordResult);
            delete[] wordResult;
        }
    }
    cout << "Solved: " << all_solved << endl;
    return num_guesses;
}

