# Duotrigordle Solver: Tanvi Bhargava (tanvib), Grayson Moyer (gmoyer)

## Summary

The game of [Duotrigordle](http://duotrigordle.com). If you haven’t already given up from trying to pronounce the name, chances are that you will probably throw in the towel after about 2 minutes of trying to actually play it. It has all of the most challenging aspects of the popular daily word puzzle, Wordle, but its complexity is amplified by requiring the player to solve 32 Wordle puzzles in parallel. We plan to fight fire with fire by implementing a parallelized solver for this epic word puzzle.

## Background
Wordle is a simple, yet challenging word game that has recently become very popular. The general premise is that players have six tries to guess a mystery 5-letter word, with clues to letters being revealed based on the guesses. If a guessed word contains a letter in the same position as the mystery word, then that letter is colored green. Also, if a guessed word contains a letter which is also contained in the mystery word, but that letter is in a different position, then it gets colored yellow. This feedback to the player is used to inform sequential guesses so that eventually they have limited down the possible words sufficiently to guess the correct mystery word. While this challenge is not terribly difficult (most players carry on streaks for many days of guessing the correct word), it does seem to be something that could be highly parallelized in a computational solver. In particular, this puzzle involves considering possible words to fit a pattern, and (especially in the beginning) this space is quite large to search through. Parallelizing this search for words could make the solver more practical in terms of computation time. Certainly if computation time is fixed (fixed-time constrained problem), then a parallelized implementation may be able to consider more words per guess and potentially find better words to solve the puzzle in fewer total guesses. All of this seems very straightforward, however, and seems to lack a real challenge from a parallelism perspective. 

Once something becomes too easy, a natural next challenge is trying to perform it many times all at once. Humans generally think in a very sequential manner, so trying to work on many versions of one thing at the same time quickly escalates the challenge. This is something that is often seen with chess champions, who sometimes play multiple games of chess (against different opponents) concurrently. Well, this same idea has already been applied to the Wordle, in the form of what is called the Duotrigordle. Instead of trying to guess one mystery 5-letter word, the Duotrigordle has players try to guess 32 5-letter words all at once. This means that each guess is applied to 32 different Wordle puzzles which are all happening simultaneously. Of course, you would need more than just 5 guesses to solve such a puzzle, and so your number of attempts is increased to 37. This essentially means that players can guess up to 5 words that do not match any of the 32 mystery words. Every other guess must match one of the mystery words if the player hopes to guess every mystery word in only 37 guesses.

The Duotrigordle is very intimidating at first, and it is generally difficult for a human player to manage the 32 Wordle boards being played at the same time. To consider each board’s feedback regarding matching letters all at once is overwhelming. This makes coming up with good guesses that incorporate all of the feedback from the different boards very challenging. However, it is worth noting that the Duotrigordle is by no means impossible. In fact, once several decent guesses have been made, it is quite feasible to begin guessing correct words aimed at a single board at a time which then inform future guesses made towards other boards, and so on. While this doesn’t always work out, sometimes the real challenge with the Duotrigordle isn’t that you can’t come up with good enough guesses, but instead it can just be very difficult to parse through the plethora of information from the 32 boards (if you have the patience to think through it, the game can be solved consistently). We anticipate a large amount of our work will be on effectively parallelizing the process of efficiently parsing through and communicating the huge amount of information involved in the Duotrigordle, even if the algorithmic difficulty of solving the puzzle turns out to be simpler than expected.

The blatant parallel nature of the Duotrigordle makes it seem like an intriguing target for a parallelized solver. There can be parallelism in terms of searching through possible words, working on each of the mystery words, and of course, combining the letter clues from each board to inform strategic guesses.

## Challenge
This project is challenging from a parallelism perspective because it requires an intelligent division of work amongst parallel processes / threads in order to maximize speedup while also working towards highly efficient gameplay that solves the Duotrigordle puzzle in as few guesses as possible (and perhaps as little wall-clock time as possible). Since there is so much data to process in this puzzle, communication and synchronization will be a huge factor, and analyzing different methods for communication (what data to send, how to send it, and when to send it) will be imperative for achieving impressive performance (with respect to a sequential solver which we will have to implement).

Our initial plan for parallelizing this problem is to assign one core to work on each Wordle board (one of the 32 mystery words to solve). Then, each core will work to come up with a list of possible 5-letter word guesses and some protocol will be used to share some of the possible guesses between all of the cores and based on the combined data from the cores, a single next guess will be decided on (perhaps in a limited number of iterations). After the guess is made, each core will update its list of possible guesses and this process will repeat. The most challenging and interesting part of this project will be designing the protocol for communication suggested guesses between the cores. For example, if a particular core is essentially positive about a word being a correct guess, then it will need to be able to communicate that certainty to the rest of the cores in order for that to be the agreed upon next guess. Also, guesses could be generated by merging several suggested guesses, such as “lemon” and “demos” being merged to create “demon” which shares 4 letters with each of those suggestions.

Note: The initial plan described above is only our first conceptualization of how we could approach this problem, and is provided here only to give an example of how parallelism will be challenging for this project.

## Resources
(What computers are you going to use? Data sets?)
Considering that this particular puzzle involves 32 simultaneously operating games of Wordle, we would like to be able to test our implementations on a machine with that many cores. Thus, we plan to make use of the PSC Bridges-2 machine to do performance testing of our Duotrigordle solver. Of course, we will be developing our implementation on the GHC machines and will only be using the PSC machine for more official tests. 

As for data sets, we will certainly need to rely on some data set containing a list of 5-letter English words. Depending on the dictionary, there are between 6,000 and 158,000 such words (quite a big spread), and we will have to decide which to include in our data set. The Stanford Graph Base has a data set of 5,757 words with 5 letters that were originally collected for playing a Jotto, a game from the 1950’s that is very similar to, and is often considered to be the original version of Wordle. This may be a good data set to start with, but we will need to consider other possible data sets that are more modern (and may include newer words).

Stanford Graph Base 5-letter words data set: https://homepage.cs.uiowa.edu/~sriram/21/fall04/words.html

## Goals & Deliverables
(Include 75%, 100%, and 125% targets)
As alluded to in the above sections, we will primarily be judging our solver based on time to solve a Duotrigordle puzzle (average time from several randomized word sets), as well as the number of guesses required to correctly solve each of the 32 boards (with a maximum of 37 allowed guesses). This performance evaluation will be with respect to a sequential solver, which we will be implementing first.

75% Goal:
* We implement a “successful” sequential Duotrigordle solver.
  * By “successful”, we mean that it is able to eventually correctly guess each of the 32 mystery words in no more than 50 total guesses (being extra generous for the 75% goal).
* We implement a parallelized Duotrigordle solver that divides the work amongst multiple cores / processors and coordinates communication between the cores to decide on the next guess.
  * Could be very simple communication, such as each core broadcasting its best guess with some likelihood metric and the highest likelihood guess is accepted as the overall guess.
* We implement a parallelized Duotrigordle solver that meets the following performance benchmarks when run on a machine with up to 128 cores:
  * It achieves at least a 4x speedup compared to the sequential solver. 
  Speedup will be evaluated in a few ways, that may include the following:  
     - Total time to solve the puzzle regardless of number of guesses  
     - Time to solve puzzle, averaged over the number of guesses (time to decide on a guess)  
     - Time to solve the puzzle, but only considering solves which were able to complete in less than a certain number of guesses (to be fair and discount cases where the game is solved quickly but with many many guesses)  
     - Time to solve the puzzle multiplied by the number of guesses made.
  * It consistently (> 95% of games) solves the puzzle in no more than 50 total guesses.


100% Goal:
* We implement a “good” sequential Duotrigordle solver.
  * By “good”, we mean that it is able to fairly consistently (>75% of games) correctly guess each of the 32 mystery words in no more than 37 total guesses.
* We implement a parallelized Duotrigordle solver that divides the work amongst multiple cores / processors and establishes a sophisticated communication protocol between the cores to decide on a *more optimal* next guess (the optimality of the next guess will be evident from the analysis of the performance metrics as discussed in the following major bullet point).
  * This will likely involve several iterations of arbitrated communication that will result in an agreed-upon guess that is either fairly good for multiple boards, or is extremely likely for a single board.
* We implement a parallelized Duotrigordle solver that meets the following performance benchmarks when run on a machine with up to 128 cores:
  * It achieves at least a 10x speedup compared to the sequential solver. Speedup will be evaluated in the same ways as previously described.
  * It fairly consistently (> 75% of games) solves the puzzle in no more than 37 total guesses.

125% Goal:
* We implement a “really good” sequential Duotrigordle solver.
  * By “really good”, we mean that it is able to consistently (>95% of games) correctly guess each of the 32 mystery words in no more than 37 total guesses.
* We implement a parallelized Duotrigordle solver that divides the work amongst multiple cores / processors and establishes a highly sophisticated communication protocol between the cores to decide on a *very optimal* next guess (the optimality of the next guess will be evident from the analysis of the performance metrics as discussed in the following major bullet point).
  * This will likely involve several iterations of arbitrated communication that will result in an agreed-upon guess that is either fairly good for multiple boards, or is extremely likely for a single board. Also, this may include a method of merging suggested guesses to generate a guess that is applicable to more boards at a time (as described in the Challenge section).
* We implement a parallelized Duotrigordle solver that meets the following performance benchmarks when run on a machine with up to 128 cores:
  * It achieves at least a 20x speedup compared to the sequential solver. Speedup will be evaluated in the same ways as previously described.
  * It consistently (> 95% of games) solves the puzzle in no more than 37 total guesses.

## Schedule

Milestones:
* 3/23: Submit Proposal
* 4/11: Checkpoint
* 4/29: Report
* 5/5: Presentation during final exam slot

Schedule (updated for checkpoint): ![Schedule Image](https://github.com/tanvib00/duotrigordle-solver/blob/main/Post_MilestoneUpdatedSchedule.png)

Link to detailed schedule google sheet: https://docs.google.com/spreadsheets/d/1NQESv9UieOrGbqEkrtgj2R3UJLaNaGAtiyOnydz4gqU/edit#gid=0

# MILESTONE
Our original schedule, with grey boxes to indicate the work we have done, is shown below. Our updated schedule has been reuploaded above.

![Old Schedule Image](https://github.com/tanvib00/duotrigordle-solver/blob/main/InitialScheduleProgress.png)

 * In one to two paragraphs, summarize the work that you have completed so far. 

Thus far, we have implemented a correct and consistent single Wordle solver. The program selects a random goal word from a dictionary of 5-letter-words, starts with a random guess from that dictionary, and guesses the goal word within 5 guesses on average. We have observed rare trial runs that take up to 9 guesses to correctly guess the goal word. The program is also capable of providing Wordle-style feedback, with values representing green, yellow, and black squares returned to the solver code following each guess. This feedback is incorporated into the solver, as it informs the solver's next guess.

 * Describe how you are doing with respect to the goals and deliverables stated in your proposal. Do you still believe you will be able to produce all your deliverables? If not, why? In your milestone writeup we want an updated list of goals that you plan to hit for the poster session.

We have not yet implemented functionality for the 32 simultaneous games of Wordle, which we had hoped to have done by this point, but this is largely due to the campus-wide break for Carnival over the last week. We will still be able to produce our deliverables, as we have a clear idea of the tasks to be completed for the project. Our goals for the final are still in line with the 100% goal scenario described in our project proposal above. However, based on feedback to our project proposal, we have a further goal of reporting more thoroughly on the different parallelization strategies we experiment with and the effects they have on our solver in our final report.

 * What do you plan to show at the poster session? Will it be a demo? Will it be a graph?

We plan to briefly demo our solver, which would demonstrate our sequential and parallelized solver's performance and time. We would also display graphs of our solvers' performance and solving time, based on trial runs held beforehand. We will also show in these graphs how speedup scales with the number of processing elements we test performance with.

 * We do not have preliminary results for our parallelized Duotrigordle at this time.
 
 * List the issues that concern you the most. Are there any remaining unknowns (things you simply don't know how to solve, or resource you don't know how to get) or is it just a matter of coding and doing the work?

As of now, it is just a matter of doing the work to get back on track to complete our project on time! 

# FINAL SUBMISSION
Please see the pdf document title Final Project Report.pdf


