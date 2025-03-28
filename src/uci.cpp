#include "uci.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> result;
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    result.push_back(item);
  }
  return result;
}

template <typename T, typename U> U Iterator<T, U>::next() {
  U item;
  if (!this->end()) {
    item = this->list[this->count];
    this->count++;
  }
  return item;
}

template <typename T, typename U> bool Iterator<T, U>::end() {
  return this->count >= this->list.size();
}

Square parse_square(std::string &square) {
  char x = square[0], y = square[1];
  return Square{uint8_t(7 - x + 'a'), uint8_t(y - '1')};
}

void parse_move(Engine &engine, std::string &command) {
  char x1 = command[0], y1 = command[1], x2 = command[2], y2 = command[3];
  Square from = Square{uint8_t(7 - x1 + 'a'), uint8_t(y1 - '1')};
  Square to = Square{uint8_t(7 - x2 + 'a'), uint8_t(y2 - '1')};
  std::vector<Move> moves;
  engine.board.get_moves(moves, opposite(engine.move.colour));

  Move best_move;
  for (int i = 0; i < moves.size(); i++) {
    bool is_move = true;
    if (command.size() == 5) {
      is_move = moves[i].type ==
                MoveType(MoveType::PromoteKnight + PromoteMap.at(command[4]));
    }
    if (moves[i].from == from && moves[i].to == to && is_move) {
      best_move = moves[i];
      break;
    }
  }

  engine.board = engine.board.make_move(best_move);
  engine.move = best_move;
}

void parse_fen(Engine &engine, Commands &commands) {
  for (size_t i = 0; i < 8; i++) {
    for (size_t j = 0; j < 8; j++) {
      engine.board.board[i][j] = EMPTY;
    }
  }
  for (auto &&piece : engine.board.pieces) {
    piece.taken = true;
  }

  FEN placement = FEN{commands.next(), 0};
  std::map<char, int> PieceMap = FENPieceMap;
  int x = 0, y = 0;
  while (!placement.end()) {
    char ch = placement.next();
    if (ch == '/') {
      x = 0;
      y++;
    } else if (isdigit(ch)) {
      x += ch - '0';
    } else {
      Piece &piece = engine.board.pieces[PieceMap[ch]];
      piece.square.x = 7 - x, piece.square.y = 7 - y;
      engine.board.board[7 - x][7 - y] = piece.id;
      piece.taken = false;
      PieceMap[ch] += 1;
      x += 1;
    }
  }

  if (commands.next() == "w") {
    engine.move.colour = Colour::Black;
  } else {
    engine.move.colour = Colour::White;
  }

  FEN castling = FEN{commands.next(), 0};
  engine.board.castling = Castling{};
  while (!castling.end()) {
    char ch = castling.next();
    if (ch == 'Q') {
      engine.board.castling.white_queenside = true;
    } else if (ch == 'q') {
      engine.board.castling.black_queenside = true;
    } else if (ch == 'K') {
      engine.board.castling.white_kingside = true;
    } else if (ch == 'k') {
      engine.board.castling.black_kingside = true;
    }
  }

  std::string command = commands.next();
  if (command != "-") {
    engine.board.double_step = parse_square(command).x;
  }

  commands.count += 2;
}

void parse_go(Engine &engine) {
  BestMove best_move = engine.search_moves(5);
  if (!best_move.end) {
    engine.board = engine.board.make_move(best_move.move);
    engine.move = best_move.move;
    std::cout << "bestmove " << engine.move.format() << std::endl;
    // std::cout << engine.board.format() << std::endl;
  }
}

void parse_position(UCI &uci, Commands &commands) {
  if (commands.next() == "fen") {
    if (!uci.started) {
      parse_fen(uci.engine, commands);
      uci.started = true;
    } else {
      commands.count += 6;
    }
  }
  if (commands.next() == "moves") {
    std::string command;
    while (!commands.end()) {
      command = commands.next();
    }
    parse_move(uci.engine, command);
  }
  // std::cout << uci.engine.board.format() << std::endl;
}

void UCI::init() {
  UCI uci = UCI{Engine::init()};

  std::cout << "id name Anchovy\n";
  std::cout << "id author JinWeiTan\n";

  std::cout << "option name Threads type spin default 1 min 1 max 1\n";
  std::cout << "option name Hash type spin default 1 min 1 max 1\n";

  std::cout << "uciok\n";

  while (true) {
    std::string input;
    std::getline(std::cin, input);
    Commands commands = Commands{split(input, ' '), 0};

    std::string command = commands.next();
    if (command == "go") {
      parse_go(uci.engine);
    } else if (command == "position") {
      parse_position(uci, commands);
    } else if (command == "isready") {
      std::cout << "readyok\n";
    } else if (command == "ucinewgame") {
      uci.engine.board = Board::init();
      uci.started = false;
    } else if (command == "bench") {
      std::string ch = commands.next();
      int depth = ch == "" ? 5 : (ch[0] - '0');
      uci.engine.bench(depth, ch != "");
    } else if (command == "display") {
      std::cout << uci.engine.board.format() << "\n";
    } else if (command == "quit") {
      exit(0);
    }
  }
}