#pragma GCC optimize "Ofast,unroll-loops,omit-frame-pointer,inline"
#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <sstream>
#include <queue>
#include <cstring>
#include <iomanip>
#include <cmath>
#include <cassert>
#include <chrono>
#include <utility>

#define SHOW_MOVE_LOG true
#define LOG_INPUT true
#define LOG_INFO true
#define AGENT_COUNT 5
#define PLAYER_COUNT 2
#define INF 1000000000
#define INF2 2000000000
#define BOARD_WIDTH 16001
#define BOARD_HEIGHT 9001
#define VISIBLITY_RANGE 2200
#define BUSTING_RANGE 1760

#define DEF_TIMER 45
#define BASE_SCORE 0.0

using namespace std;

//TODO: Replace rand with https://e...content-available-to-author-only...e.com/w/cpp/numeric/random/uniform_int_distribution

static int turn = 0;

#define Now() chrono::high_resolution_clock::now()

static struct Stopwatch
{
	chrono::high_resolution_clock::time_point c_time, c_timeout;

	void setTimeout(int us) { c_timeout = c_time + chrono::microseconds(us); }

	void Start(int timeOutMilliSeconds)
	{
		c_time = Now();
		setTimeout(timeOutMilliSeconds * 1000);
	}

	inline bool Timeout() { return Now() > c_timeout; }

	long long EllapsedMilliseconds() { return chrono::duration_cast<chrono::milliseconds>(Now() - c_time).count(); }
} stopwatch;

enum PlayerType
{
	OWN,
	FRIEND,
	ENEMY,
};

/*
  Point vector to compute 2D planar calaculation.
*/
class Point2
{
private:
	int x;
	int y;

public:
	Point2()
	{
		x = -1;
		y = -1;
	}
	Point2(int x, int y)
	{
		this->x = x;
		this->y = y;
	}

	Point2(const Point2 &other)
	{
		this->x = other.x;
		this->y = other.y;
	}

	Point2 operator=(const Point2 &other)
	{
		this->x = other.x;
		this->y = other.y;
		return *(this);
	}

	Point2 operator+(const Point2 &other)
	{
		return Point2(x + other.x, y + other.y);
	}

	Point2 operator+=(const Point2 &other)
	{
		this->x += other.x;
		this->y += other.y;
	}

	bool operator==(Point2 &other)
	{
		return this->x == other.x && this->y == other.y;
	}

	bool operator!=(Point2 &other)
	{
		return !(*this == other);
	}

	int manhattanDistance(Point2 &other)
	{
		return abs(x - other.x) + abs(y - other.y);
	}

	int dist2(Point2 &other)
	{
		int diffX = this->x - other.x;
		int diffY = this->y - other.y;
		return (diffX * diffX + diffY * diffY);
	}

	int dist(Point2 &other)
	{
		return sqrt(dist2(other));
	}

	bool isInsideMap()
	{
		return this->x >= 0 && this->y >= 0 && this->y < BOARD_HEIGHT && this->x < BOARD_WIDTH;
	}

	friend ostream& operator<< (ostream& os, const Point2 &point2) {
		os << point2.x << " " << point2.y;
		return os;
 	}
};

/*
  Any object which is positionable
*/
class Positionable
{
private:
	Point2 position;

public:
	Positionable(int x, int y)
	{
		this->position = Point2(x, y);
	}
	Positionable(const Point2 &other)
	{
		this->position = Point2(other);
	}
	void setPosition(const Point2 &position) {
		this->position = position;
	}
	Positionable operator=(const Positionable &other)
	{
		this->position = other.position;
		return (*this);
	}
};

/*
  The actionable object in the game.
*/
class Agent : Positionable
{
private:
	int id;
	int state;
	bool movable;
	bool active;
	int value;
	Point2 target;
public:
	Agent() : Positionable(-1, -1)
	{
		this->movable = true;
	}
	Agent(int x, int y, bool movable) : Positionable(x, y)
	{
		this->movable = movable;
	}
	Agent(const Point2 &other, bool movable) : Positionable(other)
	{
		this->movable = movable;
	}
	Agent operator=(const Agent &other)
	{
		this->id = other.id;
		this->movable = other.movable;
		this->active = other.active;
		return (*this);
	}
};

/*
Game Player
*/
class Player
{
private:
	PlayerType type;
	int agentCount;
	Agent agents[AGENT_COUNT];

public:
	Player();
	Player(const PlayerType type, int agentCount, int x[], int y[])
	{
		this->type = type;
		this->agentCount = agentCount;
		for (int i = 0; i < agentCount; ++i)
		{
			agents[i] = Agent(x[i], y[i], true);
		}
	}
	Player operator=(const Player &other)
	{
		this->type = other.type;
		this->agentCount = other.agentCount;
		for (int i = 0; i < agentCount; ++i)
		{
			this->agents[i] = other.agents[i];
		}
	}
};

class Move
{
private:
	string type, log;
	Point2 point2;
public:
	Move()
	{
		type = "";
		point2 = Point2(-1, -1);
	}

	Move(const string &type)
	{
		this->type = type;
	}

	Move(const string &type, const Point2 point2)
	{
		this->type = type;
		this->point2 = point2;
	}
	Move operator&=( const Move &other) {
		this->type = other.type;
		this->point2 = other.point2;
		return (*this);
	}
	void setType(const string &type) {
		this->type = type;
	}
	void setPosition(const Point2 &point2) {
		this->point2 = point2;
	}
	void setLog(const string &log) {
		this->log = log;
	}
	friend ostream& operator<< (ostream& os, const Move &move) {
		os << move.type << " " << move.point2;
		if (SHOW_MOVE_LOG) {
			os << " " << move.log;
		}
		os << endl;
		return os;
	}
};

class Solution
{
private:
	float score;

public:
	Move moves[AGENT_COUNT];
	Solution()
	{
		score = -INF;
	}
	void cloneTo(Solution &);
	float getScore() { return this->score; }
	void setScore(float score) { this->score = score; }
};

class GameState
{
private:
	Player players[PLAYER_COUNT];
	// Define resources and resource map. resources are the static in the map;
public:
	GameState();
	void initialize();
	void startTurn();
	float computeScore(const Solution &);
	void getRandomSolution(Solution &);
	void getBestMove(Solution &);
	void endTurn(Solution &);
	void cloneTo(GameState &);
	void cloneFrom(const GameState &);
};

GameState backup;
Solution tempSolution;

void GameState::initialize()
{
	stringstream input;
	int width, height;

	cin >> width >> height;
	if (LOG_INPUT) {
		input << width << " " << height << endl;
		cerr << input.str();
	}
}

void GameState::startTurn()
{
	stringstream input;

	if (LOG_INPUT) {
		cerr << input.str();
	}
}

float GameState::computeScore(const Solution &solution)
{
	return 0.0;
}

void GameState::getRandomSolution(Solution &solution)
{
	solution.setScore(this->computeScore(solution));
}

void GameState::cloneTo(GameState &other)
{
	for (int i = 0; i < PLAYER_COUNT; ++i) {
		other.players[i] = this->players[i];
	}
}

void GameState::cloneFrom(const GameState &other)
{
	for (int i = 0; i < PLAYER_COUNT; ++i) {
		this->players[i] = other.players[i];
	}
}

void Solution::cloneTo(Solution &other)
{
	other.score = score;
	for (int i = 0; i < AGENT_COUNT; i++)
	{
		other.moves[i] = this->moves[i];
	}
}

void GameState::getBestMove(Solution &solution)
{
	solution.setScore(-INF);
	stopwatch.Start(DEF_TIMER);
	int sims = 0;
	this->cloneTo(backup);

	while (!stopwatch.Timeout()) {
		this->getRandomSolution(tempSolution);

		this->cloneFrom(backup);
		sims++;
	}
	if (LOG_INFO)
		cerr << "sims " << sims << endl;
}

void GameState::endTurn(Solution &solution)
{
	for (int i = 0; i < AGENT_COUNT; i++) {
	}
}

FILE *f1;
int main()
{
	//freopen_s(&f1, "input.txt", "r", stdin);
	ios::sync_with_stdio(false);
	GameState state;
	state.initialize();
	Solution solution;

	while (1)
	{
		state.startTurn();
		state.getBestMove(solution);
		state.endTurn(solution);
		turn++;
	}
}
