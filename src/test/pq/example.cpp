// Copyright (c) 2016-2021 Daniel Frey and Dr. Colin Hirsch
// Please see LICENSE for license or visit https://github.com/taocpp/taopq/

#include "../getenv.hpp"
#include "../macros.hpp"

#include <tao/pq.hpp>

void run()
{
   // overwrite the default with an environment variable if needed
   const auto connection_string = tao::pq::internal::getenv( "TAOPQ_TEST_DATABASE", "dbname=template1" );

   // open a connection
   const auto conn = tao::pq::connection::create( connection_string );

   // execute statements directly
   conn->execute( "DROP TABLE IF EXISTS tao_example" );
   conn->execute( "CREATE TABLE tao_example ( a INTEGER PRIMARY KEY, b INTEGER, c TEXT NOT NULL )" );

   // preparing a statement is optional, but often recommended
   conn->prepare( "my_stmt", "INSERT INTO tao_example VALUES ( $1, $2, $3 )" );

   // use a transaction if needed
   {
      const auto tr = conn->transaction();

      // execute statement with parameters directly
      tr->execute( "INSERT INTO tao_example VALUES ( $1, $2, $3 )", 1, 42, "foo" );

      // execute prepared statement with parameters
      tr->execute( "my_stmt", 2, tao::pq::null, "Hello, world!" );

      tr->commit();
   }

   // insert/update/delete statements return a result which can be queried for the rows affected
   {
      const auto res = conn->execute( "my_stmt", 3, 3, "drei" );
      TEST_ASSERT( res.rows_affected() == 1 );
   }

   // queries have a result as well, it contains the returned data
   const auto res = conn->execute( "SELECT * FROM tao_example" );
   TEST_ASSERT( res.size() == 3 );

   // iterate over a result
   for( const auto& row : res ) {
      // access fields by index or (less efficiently) by name
      std::cout << row[ 0 ].as< int >() << ": " << row[ "c" ].as< std::string >() << std::endl;
   }

   // or convert a result into a container
   const auto v = res.vector< std::tuple< int, std::optional< int >, std::string > >();
}

auto main() -> int
{
   try {
      run();
   }
   // LCOV_EXCL_START
   catch( const std::exception& e ) {
      std::cerr << "exception: " << e.what() << std::endl;
      throw;
   }
   catch( ... ) {
      std::cerr << "unknown exception" << std::endl;
      throw;
   }
   // LCOV_EXCL_STOP
}
