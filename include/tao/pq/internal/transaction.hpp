// Copyright (c) 2016-2021 Daniel Frey and Dr. Colin Hirsch
// Please see LICENSE for license or visit https://github.com/taocpp/taopq/

#ifndef TAO_PQ_INTERNAL_TRANSACTION_HPP
#define TAO_PQ_INTERNAL_TRANSACTION_HPP

#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include <libpq-fe.h>

#include <tao/pq/internal/dependent_false.hpp>
#include <tao/pq/internal/gen.hpp>
#include <tao/pq/result.hpp>

namespace tao::pq
{
   class table_reader;

   template< template< typename... > class DefaultTraits >
   class basic_table_writer;

   namespace internal
   {
      class connection;
      class table_writer;

      class transaction
         : public std::enable_shared_from_this< transaction >
      {
      public:
         friend class pq::table_reader;
         friend class table_writer;

         template< template< typename... > class DefaultTraits >
         friend class pq::basic_table_writer;

      protected:
         std::shared_ptr< connection > m_connection;

         explicit transaction( const std::shared_ptr< connection >& connection );
         virtual ~transaction() = 0;

      public:
         transaction( const transaction& ) = delete;
         transaction( transaction&& ) = delete;
         void operator=( const transaction& ) = delete;
         void operator=( transaction&& ) = delete;

      protected:
         [[nodiscard]] virtual auto v_is_direct() const noexcept -> bool = 0;

         virtual void v_commit() = 0;
         virtual void v_rollback() = 0;

         virtual void v_reset() noexcept = 0;

         [[nodiscard]] auto current_transaction() const noexcept -> transaction*&;
         void check_current_transaction() const;

         template< template< typename... > class Traits, typename A >
         [[nodiscard]] auto to_traits( A&& a ) const
         {
            using T = Traits< std::decay_t< A > >;
            if constexpr( std::is_constructible_v< T, PGconn*, decltype( std::forward< A >( a ) ) > ) {
               return T( underlying_raw_ptr(), std::forward< A >( a ) );
            }
            else if constexpr( std::is_constructible_v< T, decltype( std::forward< A >( a ) ) > ) {
               return T( std::forward< A >( a ) );
            }
            else {
               static_assert( internal::dependent_false< T >, "no valid conversion from A to Traits" );
            }
         }

         [[nodiscard]] auto execute_params( const char* statement,
                                            const int n_params,
                                            const Oid types[],
                                            const char* const values[],
                                            const int lengths[],
                                            const int formats[] ) -> result;

         template< std::size_t... Os, std::size_t... Is, typename... Ts >
         [[nodiscard]] auto execute_indexed( const char* statement,
                                             std::index_sequence< Os... > /*unused*/,
                                             std::index_sequence< Is... > /*unused*/,
                                             const std::tuple< Ts... >& tuple )
         {
            const Oid types[] = { std::get< Os >( tuple ).template type< Is >()... };
            const char* const values[] = { std::get< Os >( tuple ).template value< Is >()... };
            const int lengths[] = { std::get< Os >( tuple ).template length< Is >()... };
            const int formats[] = { std::get< Os >( tuple ).template format< Is >()... };
            return execute_params( statement, sizeof...( Os ), types, values, lengths, formats );
         }

         template< typename... Ts >
         [[nodiscard]] auto execute_traits( const char* statement, const Ts&... ts )
         {
            using gen = internal::gen< Ts::columns... >;
            return execute_indexed( statement, typename gen::outer_sequence(), typename gen::inner_sequence(), std::tie( ts... ) );
         }

         [[nodiscard]] auto underlying_raw_ptr() const noexcept -> PGconn*;

      public:
         void commit();
         void rollback();
      };

   }  // namespace internal

}  // namespace tao::pq

#endif
