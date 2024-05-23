#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  return { closed_ };
}

void Writer::push( string data )
{
  if ( available_capacity() == 0 || data.empty() ) {
    return;
  }

  auto const n = min( available_capacity(), data.size() );
  if ( n < data.size() ) {
    data = data.substr( 0, n ); // 若data.size() > available_capacity()，则只取前available_capacity()个字符
  }
  data_queue_.push_back( std::move( data ) );
  num_bytes_buffered_ += n;
  num_bytes_pushed_ += n;
}

void Writer::close()
{
  closed_ = true;
}

uint64_t Writer::available_capacity() const
{
  return { capacity_ - num_bytes_buffered_ };
}

uint64_t Writer::bytes_pushed() const
{
  return { num_bytes_pushed_ };
}

bool Reader::is_finished() const
{
  return closed_ && num_bytes_buffered_ == 0;
}

uint64_t Reader::bytes_popped() const
{
  return { num_bytes_popped_ };
}

string_view Reader::peek() const
{
  if ( data_queue_.empty() ) {
    return {};
  }
  return string_view( data_queue_.front() );
}

void Reader::pop( uint64_t len )
{
  auto n = min( len, num_bytes_buffered_ );
  while ( n > 0 ) {
    // Remove up to n bytes from the front of the queue
    // 若n < data_queue_.front().size()，则只移除该元素前n个字符
    // 若该元素小于n个字符，则移除该元素，并继续移除下一个元素
    auto sz = data_queue_.front().size();
    if ( n < sz ) {
      string& front_string = data_queue_.front();
      front_string.erase(0, n); // 移除前n个字符
      // string_view view = front_string;
      // view.remove_prefix( n );
      num_bytes_buffered_ -= n;
      num_bytes_popped_ += n;
      return;
    }
    data_queue_.pop_front();
    n -= sz;
    num_bytes_buffered_ -= sz;
    num_bytes_popped_ += sz;
  }
}

uint64_t Reader::bytes_buffered() const
{
  return { num_bytes_buffered_ };
}
