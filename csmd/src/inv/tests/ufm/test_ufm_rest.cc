/*================================================================================

    csmd/src/inv/tests/ufm/test_ufm_rest.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <fstream>
#include <iostream>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <sstream>
#include <vector>

#include <boost/asio.hpp>

using std::cout;
using std::endl;
using boost::asio::ip::tcp;

int main()
{

 try
  {

  std::cout << "debug point 1" << std::endl;

  // Get a list of endpoints corresponding to the server name.
  boost::asio::io_service io_service;
  tcp::resolver resolver(io_service);
  //tcp::resolver::query query("10.7.0.39","http");
  tcp::resolver::query query("10.7.0.41","http");
  tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

  // Try each endpoint until we successfully establish a connection.
  tcp::socket socket(io_service);
  boost::asio::connect(socket, endpoint_iterator);

  std::cout << "debug point 2" << std::endl;

  // Form the request. We specify the "Connection: close" header so that the
  // server will close the socket after transmitting the response. This will
  // allow us to treat all data up until the EOF as the content.

  // fausto artico
  // note openssl -e produces three more characters that are not necessary
  // it is not clear why this is happening, but checking with curl this appears
  // openssl base64 -e <<< 'admin:123456' generates YWRtaW46MTIzNDU2Cg==
  // openssl base64 -d <<< 'YWRtaW46MTIzNDU2Cg==' produces admin:123456
  // however curl -v -u admin:123456 -X GET http://10.7.0.39/ufmRest/monitoring/attributes
  // shows Authorization: Basic YWRtaW46MTIzNDU2 and not YWRtaW46MTIzNDU2Cg==
  // the reason why the request was not authorized was because we were using YWRtaW46MTIzNDU2Cg==
  // the normal authentication processes that embeds the username and password in the URL did not work too
  // so the below request is the only only one that works on our systems, today 2015 08 23 10 29 

  boost::asio::streambuf request;
  std::ostream request_stream(&request);
  //request_stream << "GET /ufmRest/monitoring/attributes HTTP/1.1\r\n";
  request_stream << "GET /ufmRest/resources/links?cable_info=true HTTP/1.1\r\n";
  request_stream << "Authorization: Basic YWRtaW46MTIzNDU2\r\n";
  //request_stream << "Host: 10.7.0.39\r\n";
  request_stream << "Host: 10.7.0.41\r\n";
  request_stream << "Connection: close\r\n\r\n";

  // Send the request.
  boost::asio::write(socket, request);

  std::cout << "debug point 3" << std::endl;

  // Read the response status line. The response streambuf will automatically
  // grow to accommodate the entire line. The growth may be limited by passing
  // a maximum size to the streambuf constructor.
  boost::asio::streambuf response;
  boost::asio::read_until(socket, response, "\r\n");
  boost::asio::streambuf::const_buffers_type buf_1 = response.data();
  std::string response_copy_1(boost::asio::buffers_begin(buf_1), boost::asio::buffers_begin(buf_1) + response.size());
  
  std::cout << "HERE 1" << std::endl;
  std::cout << response_copy_1 << std::endl;
  std::cout << "THERE 1" << std::endl;

  std::cout << "debug point 4" << std::endl;

  // Check that response is OK.
  std::istream response_stream(&response);
  std::string http_version;
  response_stream >> http_version;
  unsigned int status_code;
  response_stream >> status_code;
  if (!response_stream || http_version.substr(0, 5) != "HTTP/")
  {
    std::cout << "Invalid response\n";
    return 1;
  }
  if (status_code != 200)
  {
    std::cout << "Response returned with status code " << status_code << "\n";
    return 1;
  }

  std::cout << "debut point 5" << std::endl;

  // Read the response headers, which are terminated by a blank line.
  boost::asio::read_until(socket, response, "\r\n\r\n");
  boost::asio::streambuf::const_buffers_type buf_2 = response.data();
  std::string response_copy_2(boost::asio::buffers_begin(buf_2), boost::asio::buffers_begin(buf_2) + response.size());

  std::cout << "HERE 2" << std::endl;
  std::cout << response_copy_2 << std::endl;
  std::cout << "THERE 2" << std::endl;
  std::cout << "Not clear why here1 and here2 are the same" << std::endl;

  // Process the response headers.
  std::string header;
  while (std::getline(response_stream, header) && header != "\r")
    std::cout << header << "\n";
  std::cout << "\n";

  // Write whatever content we already have to output.
  if (response.size() > 0)
    std::cout << &response;

  std::cout << "debug point 6" << std::endl;

  /*  
  std::string line_response;
  std::cout << std::endl;
  std::cout << "LINES RESPONSE" << std::endl;
  while( std::getline(response_stream, line_response) ){
   std::cout << line_response << std::endl;
  }
  std::cout << std::endl;
  */
 
  // opening output file
  std::string output_file_name = "output_file.txt";
  std::cout << "output file name: " << output_file_name << std::endl << std::endl;
  std::ofstream output_file(output_file_name.c_str(),std::ios::out);

  // checking if output file is open
  if ( ! output_file.is_open() )
  {

   // printing error and return
   std::cout << "Output file " << output_file_name << " not open, return"  << std::endl << std::endl;
   return 1;

  } else {

   // updating output file
   // response_copy_1 is not nedded because response_copy_2 is equal to response_copy_2
   //output_file << response_copy_1;
   output_file << response_copy_2;
   boost::system::error_code error;
   while (boost::asio::read(socket, response, boost::asio::transfer_at_least(1), error))
   {
    output_file << &response;
   }

   // closing the output file
   output_file.close();

   // checking for errors
   if (error != boost::asio::error::eof)
   {
    throw boost::system::system_error(error);
   }

  }

  std::cout << "debug point 7" << std::endl;

  // vectors with the fields
  std::size_t position_delimiter;
  std::string line;
  std::string comparing_string;
  std::vector<std::string> vector_of_the_comparing_strings;
  std::vector<std::string> vector_of_the_serial_numbers;
  std::vector<std::string> vector_of_the_part_numbers;
  std::vector<std::string> vector_of_the_hw_revisions;
  std::vector<std::string> vector_of_the_source_guids;
  std::vector<std::string> vector_of_the_source_ports;
  std::vector<std::string> vector_of_the_destination_guids;
  std::vector<std::string> vector_of_the_destination_ports;
  std::vector<std::string> vector_of_the_technologies;
  std::vector<std::string> vector_of_the_lengths;

  // setting vector of the comparing strings
  vector_of_the_comparing_strings.push_back("serial_number");           // 0
  vector_of_the_comparing_strings.push_back("part_number");             // 1
  vector_of_the_comparing_strings.push_back("hw_revision");             // 2 disappeared
  vector_of_the_comparing_strings.push_back("source_guid");             // 3
  vector_of_the_comparing_strings.push_back("source_port");             // 4
  vector_of_the_comparing_strings.push_back("destination_guid");        // 5
  vector_of_the_comparing_strings.push_back("destination_port");        // 6
  vector_of_the_comparing_strings.push_back("technology");              // 7
  vector_of_the_comparing_strings.push_back("length");                  // 8

  // opening input file
  std::string input_file_name = "output_file.txt";
  std::cout << "input file name: " << input_file_name << std::endl << std::endl;
  std::ifstream input_file(input_file_name.c_str(),std::ios::in);

  // checking if input file is open
  if ( ! input_file.is_open() )
  {

   // printing error and return
   std::cout << "Input file " << input_file_name << " not open, return" << std::endl << std::endl;
   return 1;

  } else {

   // reading the input file lines
   std::string line;
   while (std::getline(input_file, line))
   {

    // reading the line
    std::istringstream iss(line);
    std::cout << "line: " << line << std::endl;

    // cycling of the cpmaring strings
    for (unsigned int i = 0; i < vector_of_the_comparing_strings.size(); i++)
    {

     // reading comarping string
     comparing_string=vector_of_the_comparing_strings[i];
     std::cout << "comparing string: " << comparing_string << std::endl;

     // checking if comparing string is contained in the read line
     if ( line.find(comparing_string) != std::string::npos )
     {

      // printing
      std::cout << "the line contain " << comparing_string << std::endl;

      // extraction field
      position_delimiter=line.find(":"); std::cout << "position delimiter: " << position_delimiter << std::endl;
      line.erase(0,position_delimiter+3); std::cout << "line after the modification: " << line << std::endl;

      // modification field, the last is length and is particular (this work this day 2015 08 23 at 12:24) (-1 and -3)
      // modification field at cause of UFM 5.9.5 to make the parse work also for length, last field (2015 09 20 at 10:51)
      if ( i == (vector_of_the_comparing_strings.size()-1) ){
       line.erase(line.length()-3,line.length()-1); std::cout << "line after the modification: " << line << std::endl;
      } else {
       line.erase(line.length()-3,line.length()-1); std::cout << "line after the modification: " << line << std::endl;
      }
      std::cout << "";

      // updating vectors
      switch (i)
      {
       case 0:
        std::cout << "updating vector of the serial numbers" << std::endl;
        vector_of_the_serial_numbers.push_back(line);
        break;
       case 1:
        std::cout << "updating vector of the part numbers" << std::endl;
        vector_of_the_part_numbers.push_back(line);
        break;
       case 2:
        std::cout << "updating vector of the hw revisions" << std::endl;
        vector_of_the_hw_revisions.push_back(line);
        break;
       case 3:
        std::cout << "updating vector of the source guids" << std::endl;
        vector_of_the_source_guids.push_back(line);
        break;
       case 4:
        std::cout << "updating vector of the source ports" << std::endl;
        vector_of_the_source_ports.push_back(line);
        break;
       case 5:
        std::cout << "updating vector of the destination guids" << std::endl;
        vector_of_the_destination_guids.push_back(line);
        break;
       case 6:
        std::cout << "updating vector of the destination ports" << std::endl;
        vector_of_the_destination_ports.push_back(line);
        break;
       case 7:
        std::cout << "updating vector of the technologies" << std::endl;
        vector_of_the_technologies.push_back(line);
        break;
       case 8:
        std::cout << "updating vector of the lengths" << std::endl;
        vector_of_the_lengths.push_back(line);
        break;
      }

     }

    }

   }

   // printing

   /*
   for (unsigned int i = 0; i < vector_of_the_serial_numbers.size();    i++){ std::cout << "serial number:    " << vector_of_the_serial_numbers[i]    << std::endl; } std:: cout << std::endl;
   for (unsigned int i = 0; i < vector_of_the_part_numbers.size();      i++){ std::cout << "part number:      " << vector_of_the_part_numbers[i]      << std::endl; } std:: cout << std::endl;
   for (unsigned int i = 0; i < vector_of_the_hw_revisions.size();      i++){ std::cout << "hw revision:      " << vector_of_the_hw_revisions[i]      << std::endl; } std:: cout << std::endl;
   for (unsigned int i = 0; i < vector_of_the_source_guids.size();      i++){ std::cout << "source guid:      " << vector_of_the_source_guids[i]      << std::endl; } std:: cout << std::endl;
   for (unsigned int i = 0; i < vector_of_the_source_ports.size();      i++){ std::cout << "source port:      " << vector_of_the_source_ports[i]      << std::endl; } std:: cout << std::endl;
   for (unsigned int i = 0; i < vector_of_the_destination_guids.size(); i++){ std::cout << "destination guid: " << vector_of_the_destination_guids[i] << std::endl; } std:: cout << std::endl;
   for (unsigned int i = 0; i < vector_of_the_destination_ports.size(); i++){ std::cout << "destination port: " << vector_of_the_destination_ports[i] << std::endl; } std:: cout << std::endl;
   for (unsigned int i = 0; i < vector_of_the_technologies.size();      i++){ std::cout << "technologies:     " << vector_of_the_technologies[i]      << std::endl; } std:: cout << std::endl;
   for (unsigned int i = 0; i < vector_of_the_lengths.size();           i++){ std::cout << "lengths:          " << vector_of_the_lengths[i]           << std::endl; } std:: cout << std::endl;
   */

   std::cout << "serial numbers:    " << vector_of_the_serial_numbers.size()    << std::endl;
   std::cout << "part numbers:      " << vector_of_the_part_numbers.size()      << std::endl;
   std::cout << "hw revisions:      " << vector_of_the_hw_revisions.size()      << std::endl;
   std::cout << "source guids:      " << vector_of_the_source_guids.size()      << std::endl;
   std::cout << "souce ports:       " << vector_of_the_source_ports.size()      << std::endl;
   std::cout << "destination guids: " << vector_of_the_destination_guids.size() << std::endl;
   std::cout << "destination ports: " << vector_of_the_destination_ports.size() << std::endl;
   std::cout << "technologies:      " << vector_of_the_technologies.size()      << std::endl;
   std::cout << "lengths:           " << vector_of_the_lengths.size()           << std::endl;

   for (unsigned int i = 0; i < 3; i++){ std::cout << "serial number:    " << vector_of_the_serial_numbers[i]    << std::endl; } std:: cout << std::endl;
   for (unsigned int i = 0; i < 3; i++){ std::cout << "part number:      " << vector_of_the_part_numbers[i]      << std::endl; } std:: cout << std::endl;
   //for (unsigned int i = 0; i < 3; i++){ std::cout << "hw revision:      " << vector_of_the_hw_revisions[i]      << std::endl; } std:: cout << std::endl;
   for (unsigned int i = 0; i < 3; i++){ std::cout << "source guid:      " << vector_of_the_source_guids[i]      << std::endl; } std:: cout << std::endl;
   for (unsigned int i = 0; i < 3; i++){ std::cout << "source port:      " << vector_of_the_source_ports[i]      << std::endl; } std:: cout << std::endl;
   for (unsigned int i = 0; i < 3; i++){ std::cout << "destination guid: " << vector_of_the_destination_guids[i] << std::endl; } std:: cout << std::endl;
   for (unsigned int i = 0; i < 3; i++){ std::cout << "destination port: " << vector_of_the_destination_ports[i] << std::endl; } std:: cout << std::endl;
   for (unsigned int i = 0; i < 3; i++){ std::cout << "technologies:     " << vector_of_the_technologies[i]      << std::endl; } std:: cout << std::endl;
   for (unsigned int i = 0; i < 3; i++){ std::cout << "lengths:          " << vector_of_the_lengths[i]           << std::endl; } std:: cout << std::endl;

   for (unsigned int i = 0; i < 3; i++){
    std::cout << "Cable: " << i << std::endl;
    std::cout << "serial number:    " << vector_of_the_serial_numbers[i]    << std::endl;
    std::cout << "part number:      " << vector_of_the_part_numbers[i]      << std::endl;
    //std::cout << "hw revision:      " << vector_of_the_hw_revisions[i]      << std::endl;
    std::cout << "source guid:      " << vector_of_the_source_guids[i]      << std::endl;
    std::cout << "source port:      " << vector_of_the_source_ports[i]      << std::endl;
    std::cout << "destination guid: " << vector_of_the_destination_guids[i] << std::endl;
    std::cout << "destination port: " << vector_of_the_destination_ports[i] << std::endl;
    std::cout << "technologies:     " << vector_of_the_technologies[i]      << std::endl;
    std::cout << "lengths:          " << vector_of_the_lengths[i]           << std::endl;
    std:: cout << std::endl;
   }

   // closing the input file
   input_file.close();

  }

  /*
  // Read until EOF, writing data to output as we go.
  boost::system::error_code error;
  while (boost::asio::read(socket, response, boost::asio::transfer_at_least(1), error))
  //while (boost::asio::read_until(socket, response, '\n', error))
    std::cout << "line: " << &response;
  if (error != boost::asio::error::eof)
    throw boost::system::system_error(error);
  */

  }
  catch (std::exception& e)
  {
    std::cout << "Exception: " << e.what() << "\n";
  }


  std::cout << std::endl;
  std::cout << "debug piont 8, end" << std::endl;

  return 0;

}

