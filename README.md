Ludo Board Game

Table of Contents

Introduction

Project Overview

Phase I: Ludo Board Game

3.1 Pseudo Code

3.2 Operating System Concepts

3.3 Implemented Code

3.4 System Specifications

3.5 Application in Other Scenarios

Phase II: Ludo Game Implementation

4.1 Pseudo Code

4.2 Operating System Concepts

4.3 Implemented Code

4.4 System Specifications

4.5 Application in Other Scenarios

Conclusion

Group Details

Introduction

The Ludo Board Game project simulates the classic strategy game using multithreading and synchronization techniques. This project showcases operating system concepts in software development.

Project Overview

Ludo is a strategy-based board game for 2 to 4 players. The objective is to move all four tokens from start to finish based on dice rolls. This project implements a multithreaded application where each player operates as an independent thread, synchronized using mutexes and condition variables.

Phase I: Ludo Board Game

Pseudo Code

function initializePlayersThread():
    for each player in players:
        create player thread
        initialize player tokens

function playTurnThread(player_id, dice_value):
    lock board_mutex
    while current_player != player_id:
        wait on turn_cond
    if dice_value == 6:
        reset turns_without_six
    else:
        increment turns_without_six
    current_player = (current_player + 1) % MAX_PLAYERS
    broadcast turn_cond
    unlock board_mutex

Operating System Concepts

Multithreading: Each player is represented by a thread.

Mutexes: Used to protect shared resources.

Condition Variables: Ensure synchronized turns.

Thread Cancellation: Remove inactive players.

Implemented Code

pthread_mutex_t board_mutex;
pthread_cond_t turn_cond;
int current_player;
bool game_over;

void initializePlayersThread() {
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        Player player;
        player.turns_without_six = 0;
        player.is_active = true;
        players.push_back(player);
    }
}

System Specifications

Operating System: Linux

Compiler: GCC

Libraries: pthread, semaphore

Application in Other Scenarios

This implementation can be adapted for multi-user database management or other synchronized multithreaded applications.

Phase II: Ludo Game Implementation

Pseudo Code

function initializeGame():
    initialize players
    initialize board
    initialize dice

function rollDice():
    if dice is not rolling:
        generate random dice value
        handle dice roll result

Operating System Concepts

Semaphores: Control token access.

Random Number Generation: Simulate dice rolls.

Animation: Simulate token movements.

Implemented Code

void rollDice() {
    if (m_isRolling) return;
    m_isRolling = true;
    m_diceVisible = true;

    QPropertyAnimation *emergeAnim = new QPropertyAnimation(this, "dicePos");
    emergeAnim->setDuration(300);
    sequence->start(QAbstractAnimation::DeleteWhenStopped);
}

System Specifications

Operating System: Linux

Compiler: GCC

Libraries: pthread, semaphore, Qt (for GUI)

Application in Other Scenarios

This logic can be used for resource allocation in operating systems and simulation applications.

Conclusion

This project successfully demonstrates multithreading and synchronization techniques through an engaging Ludo board game.


