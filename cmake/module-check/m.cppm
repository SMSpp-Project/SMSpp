module;
#include <vector>
export module smspp_module_check;
export template< typename T >
int walk( T n )
{
 std::vector< T > v( n );
 int k = 0;
 for( auto it = v.begin() ; it != v.end() ; ++it )
  ++k;
 return( k - int( v.end() - v.begin() ) );
 }
