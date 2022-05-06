#include <set>
#include <string>
#include <iostream>
#include <fstream>
#include <iterator>
// #include <chrono>
#include <vector>
#include "mpi.h"
#include <stdlib.h>

using namespace std;

#define NUM_TRIALS 10

// #define DESIRED_DATASET_SIZE 500000
#define DESIRED_DATASET_SIZE 5757

#define MAX_GUESSES 50
#define NWORDLES 32
#define BLACK 0
#define YELLOW 1
#define GREEN 2

#define NUM_THREADS 32

/* Global variables */


typedef struct {
    double word_score;
    char best_word[6];
} best_guess_t;

/* Function to find best word based on the word's score */
void best_guess_fn(void *input_buf, void *output_buf, int *len, MPI_Datatype *datatype) {
    (void)datatype;
    best_guess_t *input = (best_guess_t *)input_buf;
    best_guess_t *output = (best_guess_t *)output_buf;

    for (int i = 0; i < *len; i++) {
        if (input[i].word_score > output[i].word_score) {
            for (int j = 0; j < 6; j++) {
                output[i].best_word[j] = input[i].best_word[j];
            }
            output[i].word_score = input[i].word_score;
        }
    }
}

/* Select random element from a data structure */
template<typename S>
std::set<string>::iterator select_random(const S &s, size_t n) {
    set<string>::iterator it = s.begin();
    // 'advance' the iterator n times
    std::advance(it,n);
    return it;
}

/* Set up the game dictionary */
void init_data(set<string> &dictionary) {
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
    srand(3);
    
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
void get_feedback(int idx, string goalWord, string guess, int *wordResult_out, vector<bool> &solved, int &solved_cnt) {
    // get feedback from game (compare guess to goalWord)
    uint correct = 0;
    int *wordResult = new int[5];
    for (int j=0; j<5; j++) {
        wordResult[j] = BLACK; // each has value of either 0,1,2 :: black,yellow,green
    }
    bool goalLetterMatches[5] = {false, false, false, false, false};

    // check for greens first
    for (int i = 0; i < guess.length(); i++) {
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
        for (int i = 0; i < 5; i++) {
            wordResult_out[i] = wordResult[i];
        }
        return;
    }

    // check for yellows
    for (int i = 0; i < guess.length(); i++) {
        if (wordResult[i] == GREEN) continue;

        char c = guess[i];
        for (int j = 0; j < goalWord.length(); j++) {
            if (c == goalWord[j] && goalLetterMatches[j] == false) {
                goalLetterMatches[j] = true;
                // update tuple with value 1 at index i
                wordResult[i] = YELLOW;
                break;
            }
        }
    }
    for (int i = 0; i < 5; i++) {
        wordResult_out[i] = wordResult[i];
    }
    return;
}

/* Reduce specific wordle's dataset based on word feedback */
void reduce_dataset(int wnum, string guess, int *wordResult, vector<set<string> > &datasets) {
    /*cout << "wnum " << i << ", goal: " << goal_words[i] << ", guess: " << guess << ", wordResult: [ ";
    for (int j = 0; j < 5; j++) {
        cout << wordResult[j] << " ";
    }
    cout << "]" << endl;*/
    // reduce list of possible guesses based on current information (wordResult)
    set<string>::iterator word_ptr = datasets[wnum].begin();
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
                    set<string>::iterator temp = word_ptr++;
                    datasets[wnum].erase(word_ptr);
                    word_ptr = temp;
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
                    set<string>::iterator temp = word_ptr++;
                    datasets[wnum].erase(word_ptr);
                    word_ptr = temp;
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
                    set<string>::iterator temp = word_ptr++;
                    datasets[wnum].erase(word_ptr);
                    word_ptr = temp;
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
                        set<string>::iterator temp = word_ptr++;
                        datasets[wnum].erase(word_ptr);
                        word_ptr = temp;
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
string select_guess(int procID, int nprocs, MPI_Op operation, MPI_Datatype dt_guess, vector<set<string> > &datasets, vector<bool> &solved) {
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

    // do this for however many wordles assigned
    int incr = NWORDLES / nprocs;
    for (int board_idx = procID; board_idx < NWORDLES; board_idx+=incr) {
        if (solved[board_idx]) {
            // board_guesses[board_idx] = (string)("flarb");
            // board_guess_scores[board_idx] = -1;
            continue; // ignore solved boards
        }

        for (set<string>::iterator word_ptr = datasets[board_idx].begin(); word_ptr != datasets[board_idx].end(); word_ptr++) { //for each word
            string word = *word_ptr;

            for (int letter_idx = 0; letter_idx < word.length(); letter_idx++) { //for each letter
                //increment letter's counter in scoring table
                int alphabet_idx = (int)(word[letter_idx] - 'a');
                letter_tally_table[alphabet_idx][letter_idx] += ((DESIRED_DATASET_SIZE/datasets[board_idx].size())/100.0);
            }
        }
    }

    // all reduce tally table, master broadcast entire table to every other proc, implicit barrier
    double common_tally_table[26][5];
    MPI_Allreduce(&letter_tally_table[0], &common_tally_table[0], 26*5, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    
    // cout << "letter tally_table: " << letter_tally_table[0][0] << endl;

    double max_score = -1;
    vector<string> best_guesses;
    // #pragma omp parallel for default(shared) shared(best_guesses, max_score) schedule(dynamic)
    for (int board_idx = procID; board_idx < NWORDLES; board_idx+=incr) { //for each board
        if (solved[board_idx]) {
            // board_guesses[board_idx] = (string)("flarb");
            // board_guess_scores[board_idx] = -1;
            continue; // ignore solved boards
        }

        int board_dataset_size = datasets[board_idx].size();

        for (set<string>::iterator word_ptr = datasets[board_idx].begin(); word_ptr != datasets[board_idx].end(); word_ptr++) { //for each word
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
                        word_score += 2.0*common_tally_table[alphabet_idx][i]; // double weighted when index matches
                        letter_tallied[i] = true;
                    }
                    else word_score += 1.0*common_tally_table[alphabet_idx][i]; // (word[i] != letter)
                }
            }
            // word_score = (double)word_score / (double)(board_dataset_size);

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

    best_guess_t best_guess_local;

    if (best_guesses.size() > 1) { // TODO: probably don't need to do this check (can just select random from size 1 vector)
        // select random from among best_guesses
        int rand_idx = rand() % best_guesses.size();
        const char *best_guess_local_buf = best_guesses[rand_idx].c_str();
        cout << "best_guess_local_buf: " << best_guess_local_buf << endl;
        for (int i = 0; i < 6; i++) {
            best_guess_local.best_word[i] = best_guess_local_buf[i];
        }
        best_guess_local.word_score = max_score;
    }
    else {
        const char *best_guess_local_buf = best_guesses[0].c_str();
        for (int i = 0; i < 6; i++) {
            best_guess_local.best_word[i] = best_guess_local_buf[i];
        }
        best_guess_local.word_score = max_score;
    }

    best_guess_t best_guess_overall;

    MPI_Allreduce(&best_guess_local, &best_guess_overall, 1, dt_guess, operation, MPI_COMM_WORLD);

    string best_word_overall = (string)(best_guess_overall.best_word);

    cout << "procID: " << procID << "Overall Best Guess: " << best_word_overall << endl;

    // if (best_guesses.size() > 1) { // TODO: probably don't need to do this check (can just select random from size 1 vector)
    //     // select random from among best_guesses
    //     chrono::microseconds us = chrono::duration_cast< chrono::microseconds >(
    //     chrono::system_clock::now().time_since_epoch());
    //     srand(static_cast<unsigned int>(us.count()));
    //     int rand_idx = rand() % best_guesses.size();
    //     // cout << "rand Best Guess: " << best_guesses[rand_idx] << endl;
    //     return best_guesses[rand_idx];
    // }
    // else {
    //     // cout << "Best Guess: " << best_guesses[0] << endl;
    //     return best_guesses[0];
    // }
    return best_word_overall;
}

/* Set up game */
void trial_setup(set<string> &dictionary, vector<set<string> > &datasets, vector<string> &goal_words, vector<bool> &solved) {
    for (int i = 0; i < NWORDLES; i++) {
            datasets[i].clear();
    }
    goal_words.clear();
    solved.clear();
    datasets.clear();

    // goal words = select randomly from data set
    for (int i=0; i<NWORDLES; i++) {
        // TODO: on second thought, I don't think we need to re-seed this random stuff every time
        // chrono::microseconds us = chrono::duration_cast< chrono::microseconds >(
        // chrono::system_clock::now().time_since_epoch());
        // cout << "dict size: " << dictionary.size() << endl;
        int r = rand() % dictionary.size();
        string goalWord = *select_random(dictionary, r);
        goal_words.push_back(goalWord);
        solved.push_back(false); // initialize all to unsolved
        
        
        datasets.push_back();
        
        copy( dictionary.begin(), dictionary.end(), inserter(datasets[i], datasets[i].begin()) ); // copy dict to each dataset

        if (NUM_TRIALS == 1) cout << "GOAL WORD " << i << ": " << goalWord << endl;
    }
}

// parallelizing over wordles - each proc to 
void compute(int procID, int nprocs, MPI_Op operation, MPI_Datatype dt_guess) {

    set<string> dictionary;
    vector<set<string> > datasets; // for 32 games, 32 dif datasets, 32 goal words
    vector<string> goal_words;
    vector<bool> solved;
    int solved_cnt;


    const int root = 0; // Set the rank 0 process as the root process
    int tag = 0;        // Use the same tag
    MPI_Status status;
    int source;

    // if (procID == root) init_data();
    init_data(dictionary);

    srand(23);

    for (int trial_num = 0; trial_num < NUM_TRIALS; trial_num++) {
        trial_setup(dictionary, datasets, goal_words, solved);

        cout << "after trial setup" << endl;

        double startTime = MPI_Wtime();

        // solve all wordles
        solved_cnt = 0;
        bool all_solved = false;
        uint num_guesses = 0; // dont technically need an array of these, bc one guess applies to all boards
        while (!all_solved && num_guesses < MAX_GUESSES) {
            string guess = select_guess(procID, nprocs, operation, dt_guess, datasets, solved);
            cout << "selected a guess" << endl;
            num_guesses++;
            if (NUM_TRIALS == 1) cout << "guess " << num_guesses << ": " << guess << endl;
            for (int i=0; i<NWORDLES; i++) {
                if (solved[i]) continue; // don't re-check word if already solved
                int wordResult[5];
                get_feedback(i, goal_words[i], guess, &wordResult[0], solved, solved_cnt);
                cout << "got feedback" << endl;
                if (solved_cnt == NWORDLES) {
                    all_solved = true;
                    break;
                }
                cout << "AHHH" << endl;
                reduce_dataset(i, guess, wordResult, datasets);
                cout << "after reduce dataset" << endl;
                // delete[] wordResult;
            }
        }

        double tm = MPI_Wtime() - startTime;

        if (all_solved) cout << "Trial " << trial_num << " solved with " << num_guesses << " guesses, in " << (double)tm/1000000 << " seconds" << endl;
        else cout << "Trial " << trial_num << " NOT SOLVED with " << num_guesses << " guesses, in " << (double)tm/1000000 << " seconds" << endl;
        
        // barrier to prevent moving on to next trial
        MPI_Barrier(MPI_COMM_WORLD);
    }
}

/* Run the game */
int main() {
    int procID;
    int nproc;
    double startTime;
    double endTime;
    // Initialize MPI
    MPI_Init(NULL, NULL);

    

    // initialize custom comparison function for word_score allreduce
    MPI_Op operation;
    MPI_Op_create(&best_guess_fn, 1, &operation);
    // operation_global = operation;

    MPI_Datatype dt_guess;
    best_guess_t dummy_guess;
    MPI_Aint displacements[2];
    MPI_Aint base_address;
    MPI_Get_address(&dummy_guess, &base_address);
    MPI_Get_address(&dummy_guess.word_score, &displacements[0]);
    MPI_Get_address(&dummy_guess.best_word, &displacements[1]);
    displacements[0] = MPI_Aint_diff(displacements[0], base_address);
    displacements[1] = MPI_Aint_diff(displacements[1], base_address);
    MPI_Datatype types[2] = {MPI_DOUBLE, MPI_CHAR};
    int lengths[2] = {1,6};
    MPI_Type_create_struct(2, lengths, displacements, types, &dt_guess);
    MPI_Type_commit(&dt_guess);
    // dt_guess_global = dt_guess;
    // Get process rank
    MPI_Comm_rank(MPI_COMM_WORLD, &procID);
    // Get total number of processes specificed at start of run
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    cout << "NumProc: " << nproc << endl;
    // Run computation
    startTime = MPI_Wtime();

    compute(procID, nproc, operation, dt_guess);
    
    endTime = MPI_Wtime();
    // Compute running time
    MPI_Finalize();
    printf("elapsed time for proc %d: %f\n", procID, endTime - startTime);

    // cout << "average time per trial = " << total_trial_time / NUM_TRIALS << endl;
    return 1;
}
