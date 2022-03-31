#include <set>
#include <string>
#include <iostream>
#include <fstream>
#include <iterator>
#include <chrono>

using namespace std;

#define MAX_GUESSES 100

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
    FILE *data_file = fopen(filename, "r");
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

    // initial guess -> random? 'adieu'

    bool solved = false;
    uint num_guesses = 0;
    while (solved == false && num_guesses < MAX_GUESSES) {
        // for now, just select random guess from list of possible next guesses?
        // eventually, figure out how to make the guess more smart
        us = chrono::duration_cast< chrono::microseconds >(
        chrono::system_clock::now().time_since_epoch());
        srand(static_cast<unsigned int>(us.count()));
        auto r = rand() % dataset.size();
        string guess = *select_random(dataset, r);
        cout << "Guess " << num_guesses << ": " << guess << endl;

        // get feedback from game (compare guess to goalWord)
            // if match, set solved = true;
        
        uint correct = 0;
        // set up 5-tuple 'wordResult'
        // each has value of either 0,1,2 :: black,yellow,green
        for (auto i = 0; i < guess.length(); i++) {
            char c = guess[i];
            if (c == goalWord[i]) {
                correct++;
                // update tuple with value 2 at index i
                continue;
            }

            for (auto j = 0; j < goalWord.length(); j++) {
                if (c == goalWord[j] && guess[j] != c) { // goal: while, guess: skill. first l should be green, second black
                    // goal: steal, guess: grass
                    // update tuple with value 1 at index i
                    break;
                }
            }
        }

        if (correct == 5) solved = true;
        

        // reduce list of possible guesses based on current information

        num_guesses++;
    }
    return num_guesses;
}
