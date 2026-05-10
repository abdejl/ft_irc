NAME    = ft_irc
CC      = c++
FLAGS   = -Wall -Wextra -Werror -std=c++98

SRC_DIR = parsing
SRCS    = $(SRC_DIR)/main.cpp $(SRC_DIR)/parser.cpp \
          $(SRC_DIR)/Client.cpp $(SRC_DIR)/cmdDispatcher.cpp

OBJS    = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CC) $(FLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS)

fclean: clean
	rm -rf $(NAME)

re: fclean all

.PHONY: all clean fclean re
