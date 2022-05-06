#include <set>
#include <string>
#include <iostream>
#include <fstream>
#include <iterator>
#include <chrono>
#include <vector>
#include <omp.h>

using namespace std;

#define NUM_TRIALS 10

#define DESIRED_DATASET_SIZE 500000
// #define DESIRED_DATASET_SIZE 5757

#define MAX_GUESSES 50
#define NWORDLES 32
#define BLACK 0
#define YELLOW 1
#define GREEN 2

#define NUM_THREADS 8

/* Global variables */
set<string> dictionary;
set<string> datasets[NWORDLES]; // for 32 games, 32 dif datasets, 32 goal words
vector<string> goal_words;
vector<bool> solved;
int solved_cnt;

/* Select random element from a data structure */
template<typename S>
auto select_random(const S &s, size_t n) {
    auto it = std::begin(s);
    // 'advance' the iterator n times
    std::advance(it,n);
    return it;
}

/* Set up the game dictionary */
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
    cout << "Dataset Size: " << dictionary.size() << endl;

    int data_set_size = dictionary.size();
    
    while(data_set_size < DESIRED_DATASET_SIZE) {
        // add a nonsense word to the dataset
        char new_word_buf[5];
        for (int i = 0; i < 5; i++) {
            int rand_idx = rand() % 26;
            char rand_char = 'a' + rand_idx;
            new_word_buf[i] = rand_char;
            // cout << "rand char: " << rand_char << endl;
        }
        string new_word(new_word_buf);
        // cout << "NEW WORD: " << new_word << endl;
        if (dictionary.find(new_word) == dictionary.end()) {
            // word not already in dataset
            dictionary.insert(new_word);
            data_set_size++;
            // cout << "added to dataset" << endl;
            // cout << "New word: " << new_word << endl;
        }
        else {
            // cout << "Word was already in dataset: " << new_word << endl;
        }
    }

    cout << "Inflated Dataset Size: " << dictionary.size() << endl;
}

/* Get feedback on a wordle guess (green/yellow/black) by comparing it to goal word */
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

/* Reduce specific wordle's dataset based on word feedback */
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

/* Select best guess based on all wordles' best guesses */
string select_guess() {
    // check all datasets and pick one
    // for now, just selects first guess from listS of possible next guesses
    // string board_guesses[NWORDLES];
    // int board_guess_scores[NWORDLES];

    // initialize the table to hold letter counts
    double letter_tally_table[26][5];
    for (int i = 0; i < 26; i++) {
        for (int j = 0; j < 5; j++) {
            letter_tally_table[i][j] = 0;
        }
    }

    // #pragma omp parallel for default(shared) shared(letter_tally_table) schedule(dynamic)
    for (int board_idx = 0; board_idx < NWORDLES; board_idx++) { //for each board
        if (solved[board_idx]) {
            // board_guesses[board_idx] = (string)("flarb");
            // board_guess_scores[board_idx] = -1;
            continue; // ignore solved boards
        }

        // parallelizing over dataset of a particular wordle
        // #pragma omp parallel {
        // for (std::set<string>::const_iterator i = datasets[board_idx].begin(); i != datasets[board_idx].end(); ++i) {
        //     #pragma omp single nowait {
        //         operate(*i);
        //     }
        // }
        auto word_ptr = datasets[board_idx].begin();
        int i = 0;
        #pragma omp parallel default(shared) shared(letter_tally_table)
        while (i < datasets[board_idx].size()) {
        //for (set<string>::iterator word_ptr = datasets[board_idx].begin(); word_ptr != datasets[board_idx].end(); ++word_ptr) {
        //for (auto word_ptr = datasets[board_idx].begin(); word_ptr != datasets[board_idx].end(); word_ptr++) { //for each word
            #pragma omp single nowait 
            {
                //auto word_ptr = i;
                string word = *word_ptr;

                for (int letter_idx = 0; letter_idx < word.length(); letter_idx++) { //for each letter
                    //increment letter's counter in scoring table
                    int alphabet_idx = (int)(word[letter_idx] - 'a');
                    #pragma omp atomic
                    letter_tally_table[alphabet_idx][letter_idx] += ((DESIRED_DATASET_SIZE/datasets[board_idx].size())/100.0);
                }
            }


            if (word_ptr != datasets[board_idx].end()) {
                word_ptr++;
            }
            else break;
            i++;
        }
        //printf("%d\n", datasets[board_idx].size());
    }

    // cout << "letter tally_table: " << letter_tally_table[0][0] << endl;

    // #pragma omp barrier

    double max_score = -1;
    vector<string> best_guesses;
    //#pragma omp parallel for default(shared) shared(best_guesses, max_score) schedule(dynamic)
    for (int board_idx = 0; board_idx < NWORDLES; board_idx++) { //for each board
        if (solved[board_idx]) {
            // board_guesses[board_idx] = (string)("flarb");
            // board_guess_scores[board_idx] = -1;
            continue; // ignore solved boards
        }

        int board_dataset_size = datasets[board_idx].size();

        for (auto word_ptr = datasets[board_idx].begin(); word_ptr != datasets[board_idx].end(); word_ptr++) { //for each word
            string word = *word_ptr;
            double word_score = 0;
            bool letter_tallied[5] = {false, false, false, false, false};

            for (int letter_idx = 0; letter_idx < word.length(); letter_idx++) { //for each letter in word
                if (letter_tallied[letter_idx]) continue;
                char letter = word[letter_idx];
                int alphabet_idx = (int)(word[letter_idx] - 'a');
                //look up letter's counts in the scoring table
                for (int i = 0; i < word.length(); i++) { // for each index of that letter in the letter_tally_table
                    if (word[i] == letter) {
                        word_score += 2.0*letter_tally_table[alphabet_idx][i]; // double weighted when index matches
                        letter_tallied[i] = true;
                    }
                    else word_score += 1.0*letter_tally_table[alphabet_idx][i]; // (word[i] != letter)
                }
            }
            // word_score = (double)word_score / (double)(board_dataset_size);

            #pragma omp critical(updateMaxValue)
            {
                if (word_score > max_score) {
                    max_score = word_score;
                    best_guesses.clear();
                    best_guesses.push_back(word);
                }
                else if (word_score == max_score) {
                    best_guesses.push_back(word); // some vector to store each best guess if there are multiple with same score
                }
            }
                
        }
    }

    if (best_guesses.size() > 1) { // TODO: probably don't need to do this check (can just select random from size 1 vector)
        // select random from among best_guesses
        chrono::microseconds us = chrono::duration_cast< chrono::microseconds >(
        chrono::system_clock::now().time_since_epoch());
        srand(static_cast<unsigned int>(us.count()));
        int rand_idx = rand() % best_guesses.size();
        // cout << "rand Best Guess: " << best_guesses[rand_idx] << endl;
        return best_guesses[rand_idx];
    }
    else {
        // cout << "Best Guess: " << best_guesses[0] << endl;
        return best_guesses[0];
    }

    // double overall_max_score = -1;
    // string overall_best_guess;

    // for (int board_idx = 0; board_idx < NWORDLES; board_idx++) {
    //     if (solved[board_idx]) continue;

    //     string candidate_word = board_guesses[board_idx];
    //     int raw_score = board_guess_scores[board_idx];
    //     double score = (double)raw_score / (double)(datasets[board_idx].size());
    //     if (score > overall_max_score) {
    //         overall_max_score = score;
    //         overall_best_guess = candidate_word;
    //     }
    //     if (NUM_TRIALS == 1) {
    //         cout << "dsize " << board_idx << ": " << datasets[board_idx].size() << ", ";
    //         cout << "candidate word: " << candidate_word << ", score: " << score << endl;
    //     }
    // }
    return (string)"flarb";
}

/* Run the game */
int main() {
  printf("There are %d processors\n", omp_get_num_procs());
    omp_set_num_threads(NUM_THREADS);

    init_data();
    double total_trial_time = 0;

    for (int trial_num = 0; trial_num < NUM_TRIALS; trial_num++) {
        for (int i = 0; i < NWORDLES; i++) {
            datasets[i].clear();
        }
        goal_words.clear();
        solved.clear();

        // goal words = select randomly from data set
        for (auto i=0; i<NWORDLES; i++) {
            // TODO: on second thought, I don't think we need to re-seed this random stuff every time
            chrono::microseconds us = chrono::duration_cast< chrono::microseconds >(
            chrono::system_clock::now().time_since_epoch());
            srand(static_cast<unsigned int>(us.count()));
            auto r = rand() % dictionary.size();
            string goalWord = *select_random(dictionary, r);
            goal_words.push_back(goalWord);
            solved.push_back(false); // initialize all to unsolved

            copy( dictionary.begin(), dictionary.end(), inserter(datasets[i], datasets[i].begin()) ); // copy dict to each dataset

            if (NUM_TRIALS == 1) cout << "GOAL WORD " << i << ": " << goalWord << endl;
        }

        chrono::microseconds startTime = chrono::duration_cast< chrono::microseconds >(
            chrono::system_clock::now().time_since_epoch());

        // solve all wordles
        solved_cnt = 0;
        bool all_solved = false;
        uint num_guesses = 0; // dont technically need an array of these, bc one guess applies to all boards
        while (!all_solved && num_guesses < MAX_GUESSES) {
            string guess = select_guess();
            num_guesses++;
            if (NUM_TRIALS == 1) cout << "guess " << num_guesses << ": " << guess << endl;
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

        chrono::microseconds tm = chrono::duration_cast< chrono::microseconds >(
            chrono::system_clock::now().time_since_epoch()) - startTime;

        if (all_solved) cout << "Trial " << trial_num << " solved with " << num_guesses << " guesses, in " << (double)tm.count()/1000000 << " seconds" << endl;
        else cout << "Trial " << trial_num << " NOT SOLVED with " << num_guesses << " guesses, in " << (double)tm.count()/1000000 << " seconds" << endl;
        total_trial_time += (double)tm.count()/1000000;
    }
    cout << "average time per trial = " << total_trial_time / NUM_TRIALS << endl;
    return 1;
}

