NAME = ft_irc

CXX = c++

CXXFLAGS = -Wall -Wextra -Werror -std=c++98

SRCS = main.cpp parser.cpp

OBJ = $(SRCS:%.cpp=%.o)

all : $(NAME)

$(NAME) : $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean :
	rm -rf $(OBJ)

fclean : clean
	rm -rf $(NAME)

re : fclean all
