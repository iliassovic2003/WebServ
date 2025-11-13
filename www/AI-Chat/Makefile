PURPLE    = \033[0;35m
BLUE      = \033[0;34m
GREEN     = \033[0;32m
RED       = \033[0;31m
YELLOW    = \033[0;33m
RESET     = \033[0m

NAME = ssMan

SRC = ssMan.cpp
OBJ = $(SRC:.cpp=.o)

CXX = c++ -std=c++98 -Wall -Wextra -Werror

all: $(NAME)

$(NAME): $(OBJ)
	@echo "$(GREEN)🔧 Creating The Session Manager...$(RESET)"
	@$(CXX) $(OBJ) -o $(NAME)

clean:
	@echo "$(YELLOW)🧹 Cleaning The Objectfile...$(RESET)"
	@rm -rf $(OBJ)

fclean: clean
	@echo "$(RED)🧹 Cleaning The Session Manager...$(RESET)"
	@rm -rf $(NAME)

full: all clean

.PHONY: all clean full
.SECONDARY: $(OBJ)