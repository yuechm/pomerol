#include "LatticeReader.h"
#include <fstream>

namespace Pomerol{

LatticeReader::LatticeReader(){
};

int LatticeReader::readinFromJSON(const std::string filename)
{
  Json::Reader reader;
  std::ifstream in;
  in.open(filename.c_str());
  try
  {
    bool parsingSuccessful = reader.parse( in, *root );
    if ( !parsingSuccessful )
  	{
		std::cout  << "Failed to parse configuration\n";
		std::cout << reader.getFormatedErrorMessages();
        return 1;
  	}
  }
  catch (std::exception ErrorException)
  	{
		std::cout << ErrorException.what() << std::endl;
		exit(1);
  	}
  in.close();
  return 0;
}

} // end of namespace Pomerol
