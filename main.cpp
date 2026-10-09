#include <iostream>
#include <string>

int main() {
  
  int maxi = 30;
  int maxj = 80;
  std::string buffer = "";

  std::cout << "\x1b[2J"; 

  std::cout << R"cactus(
         \ \ | | / / 
       _.-'''''''''-._
    .'  /  /  |  \  \  '.
  \/  \/ \/  \|/  \/ \/  \/
  |   |   |   |   |   |   |
 \|  \|  \|  \|/  |/  |/  |/
  |   |   |  \|/  |   |   |
 \|  \|  \|  \|/  |/  |/  |/
   \   \  \   |   /  /   /
    '.  \  \  |  /  /  .'
       '-._________.-'
       [_____________]
        \___________/
    ~~~~~~~~~~~~~~~~~~~~~
)cactus";

  std::cout << "\n";

  for (int i = 0; i < maxi; i++)
  {

    if (i == 0) 
    {
      

      for (int j = 0; j < maxj; j++)
      {
        std::cout << "_";
      }
    
    
    }

    std::cout << "\n";

    for (int j = 0; j < maxj; j++)
    {
      if (j == 0)
      {
        std::cout << "|";
      }
      else {
        std::cout << " ";
      }
      if (j >= maxj - 1)
      {
        std::cout << "|";

      }
    }
    
    if (i >= 29) 
    {
    

      std::cout << "\n";
      std::cout << "|";      
      
      for (int j = 0; j < maxj - 1; j++)
      {
        std::cout << "_";
      }
    
      std::cout << "|";      
    
    }



  }

  std::cout << "\n";
  std::cout << "\x1b[24;15H";
  std::cout << "Cactus!\n" << std::flush;
  std::cout << "\x1b[25;15H";


  while (buffer != "exit") {

    std::cout << "\x1b[25;16H"; 
    std::cout << "\x1b[2K" << std::flush;
    std::cout << ">";
    std::cin >> buffer;
    std::cout << "\x1b[28;15H";
    std::cout << "\x1b[2K" << std::flush;
    std::cout << buffer << std::endl << std::flush;
    std::cout <<"\x1b[70;0H";

  }


  return 0;
}
