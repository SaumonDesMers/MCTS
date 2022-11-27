#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <random>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <bitset>

#define int128(x) static_cast<__int128_t>(x)
#define FULL_ONE_MASK ((int128(0x1ffffffffff) << 40) | int128(0xffffffffff))

using namespace std;

typedef __int128_t Board;
typedef __int16_t BigBoard;

/*
	Smalls boards index in BigBoard:
	 0 | 1 | 2
	---|---|---
	 3 | 4 | 5
	---|---|---
	 6 | 7 | 8

	Cells index in Board:
	 0  1  2 |  9 10 11 | 18 19 20
	 3  4  5 | 12 13 14 | 21 22 23
	 6  7  8 | 15 16 17 | 24 25 26
	---------|----------|---------
	27 28 29 | 36 37 38 | 45 46 47
	30 31 32 | 39 40 41 | 48 59 50
	33 34 35 | 42 43 44 | 51 52 53
	---------|----------|---------
	54 55 56 | 63 64 65 | 72 73 74
	57 58 59 | 66 67 68 | 75 76 77
	60 61 62 | 69 70 71 | 78 79 80

	Small game distribution in Board:
	                                                 board 8   board 7   board 6   board 5   board 4   board 3   board 2   board 1   board 0
	00000000000000000000000000000000000000000000000 000000000 000000000 000000000 000000000 000000000 000000000 000000000 000000000 000000000

	Mask to test if board is finished:
	A B C
	D E F --> I H G F E D C B A
	G H I

	ABC = 000000111 = 0x7
	DEF = 000111000 = 0x38
	GHI = 111000000 = 0x1c0
	ADG = 001001001 = 0x49
	BEH = 010010010 = 0x92
	CFI = 100100100 = 0x124
	AEI = 100010001 = 0x111
	CEG = 001010100 = 0x54

*/

int stateId = 0;

int random(int min, int max) {
   static bool first = true;
   if (first) {  
      srand(time(NULL));
      first = false;
   }
   return min + rand() % (max - min);
}

struct Timer {
	clock_t time_point;

	Timer() { set(); }

	void set() { time_point = clock(); }

	double diff(bool reset = true) {
		clock_t next_time_point = clock();
		clock_t tick_diff = next_time_point - time_point;
		double diff = (double)(tick_diff) / CLOCKS_PER_SEC * 1000;
		if (reset)
			time_point = next_time_point;
		return diff;
	}
} timer;

int gameIndexToStrIndex[81] = {
	0,2,4,22,24,26,44,46,48,
	8,10,12,30,32,34,52,54,56,
	16,18,20,38,40,42,60,62,64,
	88,90,92,110,112,114,132,134,136,
	96,98,100,118,120,122,140,142,144,
	104,106,108,126,128,130,148,150,152,
	176,178,180,198,200,202,220,222,224,
	184,186,188,206,208,210,228,230,232,
	192,194,196,214,216,218,236,238,240
};

string indexToPos[81] = {
	"0 0","0 1","0 2","1 0","1 1","1 2","2 0","2 1","2 2",
	"0 3","0 4","0 5","1 3","1 4","1 5","2 3","2 4","2 5",
	"0 6","0 7","0 8","1 6","1 7","1 8","2 6","2 7","2 8",
	"3 0","3 1","3 2","4 0","4 1","4 2","5 0","5 1","5 2",
	"3 3","3 4","3 5","4 3","4 4","4 5","5 3","5 4","5 5",
	"3 6","3 7","3 8","4 6","4 7","4 8","5 6","5 7","5 8",
	"6 0","6 1","6 2","7 0","7 1","7 2","8 0","8 1","8 2",
	"6 3","6 4","6 5","7 3","7 4","7 5","8 3","8 4","8 5",
	"6 6","6 7","6 8","7 6","7 7","7 8","8 6","8 7","8 8",
};

int posToIndex[9][9] = {
	{ 0,  1,  2,  9, 10, 11, 18, 19, 20},
	{ 3,  4,  5, 12, 13, 14, 21, 22, 23},
	{ 6,  7,  8, 15, 16, 17, 24, 25, 26},
	{27, 28, 29, 36, 37, 38, 45, 46, 47},
	{30, 31, 32, 39, 40, 41, 48, 59, 50},
	{33, 34, 35, 42, 43, 44, 51, 52, 53},
	{54, 55, 56, 63, 64, 65, 72, 73, 74},
	{57, 58, 59, 66, 67, 68, 75, 76, 77},
	{60, 61, 62, 69, 70, 71, 78, 79, 80},
};

template<class Mask>
string mtos(Mask mask, size_t size) {
	string str;
	for (size_t i = 0; i < size; i++) {
		str.insert(0, to_string(int(mask & 1)));
		mask >>= 1;
	}
	return str;
}

struct Game {
	BigBoard myBigBoard;
	BigBoard oppBigBoard;
	Board myBoard;
	Board oppBoard;
	int myTurn;
	int lastAction;
	vector<int> validAction;
	int validActionAlreadyComputed;

	Game(BigBoard _myBigBoard, BigBoard _oppBigBoard, Board _myBoard, Board _oppBoard, int _myTurn, int _lastAction) :
		myBigBoard(_myBigBoard),
		oppBigBoard(_oppBigBoard),
		myBoard(_myBoard),
		oppBoard(_oppBoard),
		myTurn(_myTurn),
		lastAction(_lastAction),
		validAction(vector<int>()),
		validActionAlreadyComputed(false) {}

	Game(const Game &src) :
		myBigBoard(src.myBigBoard),
		oppBigBoard(src.oppBigBoard),
		myBoard(src.myBoard),
		oppBoard(src.oppBoard),
		myTurn(src.myTurn),
		lastAction(src.lastAction),
		validAction(src.validAction),
		validActionAlreadyComputed(src.validActionAlreadyComputed) {}

	Game &operator=(const Game &src) {
		myBigBoard = src.myBigBoard;
		oppBigBoard = src.oppBigBoard;
		myBoard = src.myBoard;
		oppBoard = src.oppBoard;
		myTurn = src.myTurn;
		lastAction = src.lastAction;
		validAction = src.validAction;
		validActionAlreadyComputed = src.validActionAlreadyComputed;
		return *this;
	}

	bool isSmallBoardFinal(int i) {
		return (((myBigBoard | oppBigBoard) >> i) & 1);
	}

	Board smallBoardMask(int i) {
		return (int128(0x1ff) << (i * 9));
	}

	Board getUniqueSmallBoard(Board board, int i) {
		return (board >> (i * 9)) & 0x1ff;
	}

	template<class Mask>
	int boardIsFinal(Mask board) {
		// the board must be at the right of the mask
		Mask winMask[8] = {0x7,0x38,0x1c0,0x49,0x92,0x124,0x111,0x54};

		for (size_t i = 0; i < 8; i++) {
			// maybe test "!(winMask[i] ^ (board & winMask))" to see if it's faster
			if (winMask[i] == (board & winMask[i]))
				return 1;
		}
		return 0;
	}

	vector<int> possibleActions() {
		if (!validActionAlreadyComputed) {
			validAction.reserve(81);

			Board freeCellMask = 0;
			// set all finished small board to 1 as you can't play on it anymore
			// starting from the last one
			for (ssize_t i = 8; i >= 0; i--) {
				if (isSmallBoardFinal(i)) {
					freeCellMask &= 0x1ff; // add 111111111 to the right
				}
				freeCellMask <<= 9;
			}
			// cumule finished small board with the two players board
			freeCellMask = ~(freeCellMask | myBoard | oppBoard);

			// get valid action by filtering only the free cells in the small board where you are forced to play
			// but if there is no last action: juste play in the middle
			Board validActionMask = (freeCellMask & (lastAction == -1 ? int128(1) << 40 : smallBoardMask(lastAction % 9)));

			// if there is no valid action: play where you want
			validActionMask = validActionMask ? validActionMask : freeCellMask;

			int index = 0;
			// loop while there is still a 1 (i.e. a free space)
			while (validActionMask) {
				if (validActionMask & 1)
					validAction.push_back(index);
				validActionMask >>= 1;
				index++;
			}

			validActionAlreadyComputed = true;
		}
		return validAction;
	}

	void play(int action) {
		Board actionMask = (int128(1) << action);
		int smallBoardIndex = action / 9;
		if (myTurn) {
			myBoard |= actionMask;
			int result = boardIsFinal(getUniqueSmallBoard(myBoard, smallBoardIndex));
			myBigBoard |= result << smallBoardIndex;
		}
		else {
			oppBoard |= actionMask;
			int result = boardIsFinal(getUniqueSmallBoard(oppBoard, smallBoardIndex));
			oppBigBoard |= result << smallBoardIndex;
		}

		myTurn = !myTurn;
		lastAction = action;
		validAction.clear();
		validActionAlreadyComputed = false;
	}

	bool final() {
		possibleActions();
		if (validAction.empty())
			return true;
		return boardIsFinal(myBigBoard) || boardIsFinal(oppBigBoard);
	}

	float result() {
		if (boardIsFinal(myBigBoard))
			return 1;
		if (boardIsFinal(oppBigBoard))
			return 0;
		return 0.5;
	}

	void log() {
		string str("\
. . . | . . . | . . .\n\
. . . | . . . | . . .\n\
. . . | . . . | . . .\n\
------|-------|------\n\
. . . | . . . | . . .\n\
. . . | . . . | . . .\n\
. . . | . . . | . . .\n\
------|-------|------\n\
. . . | . . . | . . .\n\
. . . | . . . | . . .\n\
. . . | . . . | . . ."
		);
		Board myBoardTmp = myBoard;
		Board oppBoardTmp = oppBoard;
		for (size_t i = 0; i < 81; i++) {

			if (myBoardTmp & 1)
				str[gameIndexToStrIndex[i]] = 'o';
			else if (oppBoardTmp & 1)
				str[gameIndexToStrIndex[i]] = 'x';
			
			myBoardTmp >>= 1;
			oppBoardTmp >>= 1;
		}
		cerr << "myTurn = " << myTurn << "  lastAction = " << (lastAction != -1 ? indexToPos[lastAction] : "none") << endl;
		cerr << str << endl;
	}
};

struct State {
	float value;
	int nb_of_visit;
	Game game;
	State *parent;
	vector<State *> children;
	int id;

	State(Game __game, State *__parent) : value(0), nb_of_visit(0), game(__game), parent(__parent), id(stateId++) {}
	~State() {
		for (size_t i = 0; i < children.size(); i++)
			delete children[i];
	}

	State *expand() {
		// if final state return current state
		if (game.final())
			return this;

		// get all valid action
		vector<int> action = game.possibleActions();

		// create games for each valid action
		vector<Game> nextGame;
		for (size_t i = 0; i < action.size(); i++) {
			Game g = game;
			g.play(action[i]);
			nextGame.push_back(g);
		}
		
		// detect if there is a final state
		Game *finalGame = NULL;
		for (size_t i = 0; i < nextGame.size(); i++) {
			if (nextGame[i].final()) {
				finalGame = &nextGame[i];
				break;
			}
		}

		// if there is a final state: expand only this one
		if (finalGame != NULL) {
			children.push_back(new State(*finalGame, this));
		}
		// else: expand all next state
		else {
			for (size_t i = 0; i < nextGame.size(); i++)
				children.push_back(new State(nextGame[i], this));
		}
		return children[0];
	}

	float UCB1() {
		if (parent == NULL)
			return -1;
		
		if (nb_of_visit == 0)
			return INFINITY;
		
		return (value / nb_of_visit) + 2 * sqrt(::log(parent->nb_of_visit) / nb_of_visit);
	}

	State *maxUCB1Child() {
		State *child = NULL;
		float maxUCB1 = 0;
		for (size_t i = 0; i < children.size(); i++) {
			float UCB1 = children[i]->UCB1();
			if (UCB1 > maxUCB1) {
				maxUCB1 = UCB1;
				child = children[i];
			}
		}
		return child;
	}

	void backpropagate(float __value) {
		nb_of_visit++;
		value += __value;
		if (parent != NULL)
			parent->backpropagate(__value);
	}

	float rollout(bool debug = false) {
		Game g = game;
		while (!g.final()) {
			vector<int> actions = g.possibleActions();
			for (size_t i = 0; i < actions.size(); i++) {
				Game test = g;
				test.play(actions[i]);
				if (test.final()) {
					if (debug) {
						std::cerr << "best play -> " << indexToPos[actions[i]] << endl;
						test.log();
					}
					return test.result();
				}
			}
			int randIndex = random(0, actions.size());
			g.play(actions[randIndex]);
			if (debug) {
				std::cerr << "random play -> " << indexToPos[actions[randIndex]] << endl;
				g.log();
				std::cerr << endl;
			}
		}
		return g.result();
	}

	State *maxAverageValueChild() {
		if (children.empty())
			return NULL;
		State *child = children[0];
		float maxAverageValue = -1;
		for (size_t i = 0; i < children.size(); i++) {
			float averageValue = children[i]->value / children[i]->nb_of_visit;
			if (averageValue > maxAverageValue) {
				maxAverageValue = averageValue;
				child = children[i];
			}
		}
		return child;
	}

	void log() {
		cerr << "State{t=" << setw(7) << left << value <<
            ",n=" << setw(5) << left << nb_of_visit <<
            ",av=" << setw(10) << left << value / nb_of_visit <<
            ",action=" << indexToPos[game.lastAction] <<
            "}" << endl;
	}

};

void dump_node(State *node, std::ofstream & myfile, int deep) {
    if (--deep <= 0)
        return;
    myfile << '\t' << node->id << "[label=\"[" << indexToPos[node->game.lastAction] << "]\n"
    <<"P=" << node->game.myTurn << "W=" << node->value << ";V=" << node->nb_of_visit<< '\"';
    myfile <<"]\n";
    for(int i = 0; i < node->children.size(); i++) {
        myfile << '\t' << node->id << " -> " << node->children[i]->id << std::endl;
    }
    for (int i = 0; i < node->children.size(); i++) {
        dump_node(node->children[i], myfile, deep);
    }
}

void dump_tree(std::string fileName, State *root) {
    std::ofstream myfile;
	myfile.open(fileName);
    myfile << "strict digraph {\n";
    myfile << "\tnode [shape=\"rect\"]\n";

    dump_node(root, myfile, 5);

    myfile << "}";
    myfile.close();
}

State *opponentPlay(State *state, int action) {
	for (size_t i = 0; i < state->children.size(); i++) {
		if (action == state->children[i]->game.lastAction)
			return state->children[i];
	}
	return state;
}

void readInput(int &oppAction, vector<int> &validAction) {
	int opp_row; int opp_col;
	cin >> opp_row >> opp_col; cin.ignore();
	oppAction = opp_row != -1 ? posToIndex[opp_row][opp_col] : -1;
	// validAction.clear();
	// int valid_action_count;
	// cin >> valid_action_count; cin.ignore();
	// for (int i = 0; i < valid_action_count; i++) {
	//     int row; int col;
	//     cin >> row >> col; cin.ignore();
	//     validAction.push_back(posToIndex[row][col]);
	// }
}

State *mcts(State *initialState, Timer start, float timeout) {
	State *current;
    int nbOfSimule = 0;

	// if (initialState->children.empty())
	// 	initialState->expand();
	while (true) {
		
        double diff = start.diff(false);
        if (diff > timeout)
            break;

		current = initialState;

		while (!current->children.empty())
			current = current->maxUCB1Child();

		if (current->nb_of_visit > 0) {
			State *firstChild = current->expand();
			if (firstChild != NULL)
				current = firstChild;
		}

		float value = current->rollout();

		current->backpropagate(value);

        nbOfSimule++;
	}
    cerr << "nb of simule = " << nbOfSimule << endl;
	return initialState->maxAverageValueChild();
}

State *mcts(State *initialState, int maxIter) {
	State *current;
    int nbOfSimule = 0;

	// if (initialState->children.empty())
	// 	initialState->expand();
	while (nbOfSimule < maxIter) {

		current = initialState;

		while (!current->children.empty())
			current = current->maxUCB1Child();

		if (current->nb_of_visit > 0) {
			current = current->expand();
		}

		float value = current->rollout();

		current->backpropagate(value);

        nbOfSimule++;
	}
    cerr << "nb of simule = " << nbOfSimule << endl;
	return initialState->maxAverageValueChild();
}

/*
void rolloutTest(Grid board) {
	cerr << "--- rollout test ---" << endl;
	Game game = Game(board, 1, npos);
	State *state = new State(game, NULL);

	state->game.log();
	cerr << endl;

	float val = state->rollout(true);
	cerr << "\nvalue = " << val << endl;
	delete state;
	cerr << "--------------------" << endl;
}

void mctsTest(Grid board) {
	cerr << "---- mcts  test ----" << endl;
	Game game = Game(board, 1, npos);
	State *state = new State(game, NULL);

	state->game.log();
	cerr << endl;

	State *child = mcts(state, 1000);

	for (size_t i = 0; i < state->children.size(); i++)
		state->children[i]->log();
	child->game.log();
	delete state;
	cerr << "--------------------" << endl;
}*/

int main(int ac, char *av[]) {

	Game game = Game(0, 0, 0, 0, 0, -1);

	// game.play(9);

	game.log();

	vector<int> validAction = game.possibleActions();
	for (size_t i = 0; i < validAction.size(); i++)
		cerr << "(" << validAction[i] << ") ";
	cerr << endl;

	return 0;
}

// int main() {
// 	int step = 0;

// 	int oppAction;
// 	vector<int> validAction;

// 	Game initialGame = Game(0, 0, 0, 0, 1, -1);
// 	State *initialState = new State(initialGame, NULL);

//     State *current = initialState;

//     while (1) {
// 		readInput(oppAction, validAction);
//         Timer start;

// 		if (step == 0) {
// 			if (oppAction != -1) {
// 				current->game.oppBoard &= 1 << oppAction;
// 			}
// 		}
// 		else {
// 			current = opponentPlay(current, oppAction);
// 		}

//         current->game.log();
//         cerr << endl;

//         // my play
//         State *child = mcts(current, start, 75);

// 		// dump_tree("tree.dot", current);

//         cerr << "simule time " << start.diff() << endl;

//         for (size_t i = 0; i < current->children.size(); i++)
//             current->children[i]->log();
        
//         if (child == NULL) {
//             cerr << "mcts did not return any action" << endl;
//             cout << indexToPos[validAction[0]] << endl;
//         }
//         else {
//             cout << indexToPos[child->game.lastAction] << endl;
//             current = child;
//         }
//         current->game.log();
//         cerr << endl;
// 		step++;
//     }
// }