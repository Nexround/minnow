#include "reassembler.hh"
#include <iostream>
#include <string>
using namespace std;
int findFirstDifference( const std::string& str1, const std::string& str2 )
{
  uint64_t index = 0;
  while ( index < str1.length() && index < str2.length() ) {
    if ( str1[index] != str2[index] )
      break; // 找到第一个不同的字符，退出循环
    ++index;
  }

  // 检查哪个字符串更短
  if ( index == str1.length() && index < str2.length() )
    return index; // str1结束，str2未结束
  else if ( index == str2.length() && index < str1.length() )
    return index; // str2结束，str1未结束
  else if ( index < str1.length() && index < str2.length() )
    return index; // 两个字符串在该位置不同

  return -1; // 字符串完全相同或其中一个为空
}

uint64_t end_idx( std::pair<uint64_t, std::string> t )
{
  return ( ( t.first + t.second.size() ) == 0 ) ? 0 : ( t.first + t.second.size() - 1 );
}

void Reassembler::update_first_unacceptable_idx()
{
  // 更新 first_unacceptable_idx
  uint64_t available_capacity = output_.writer().available_capacity();
  first_unacceptable_idx = first_unassembled_idx + available_capacity;
}

void Reassembler::overlap_process( uint64_t first_index,
                                   std::string data,
                                   std::map<uint64_t, std::string>& pending_map )
{
  // index相同的情况？
  // 首先检查是否有重叠的部分
  if ( data.size() == 0 ) {
    return;
  }
  auto end_of_this = ( ( first_index + data.size() ) == 0 ) ? 0 : ( first_index + data.size() - 1 );
  auto available_capacity = output_.writer().available_capacity();
  auto map_size = bytes_pending();
  auto candidate = std::pair<uint64_t, std::string>( first_index, data );
  // auto c_idx = candidate.first;
  // auto c_string = candidate.second;
  update_first_unacceptable_idx();
  // 若该字符串的范围超出了最大范围，则直接返回
  if ( first_index >= first_unacceptable_idx ) {
    return;
  }
  // 若该字符串部分超出了最大范围，则截取该字符串
  if ( first_index < first_unacceptable_idx && end_of_this >= first_unacceptable_idx ) {
    auto overflow_length = end_of_this - first_unacceptable_idx + 1;
    auto rm_idx = candidate.second.size() - overflow_length;
    candidate.second.erase( rm_idx, std::string::npos );
    end_of_this = first_unacceptable_idx - 1;
  }
  // 若该字符串包含了first_unassembled_idx，则从first_unassembled_idx开始截取
  if ( candidate.first < first_unassembled_idx && end_of_this >= first_unassembled_idx ) {
    candidate.second.erase( 0, first_unassembled_idx - candidate.first );
    candidate.first = first_unassembled_idx;
  }
  // 对插入前后的元素进行检查
  // 原则：若要调整长度，则调整当前字符串

  // 获取插入位置前后的元素
  // 首先检查该字符串是否与前方字符串重叠
  auto nextIter = pending_map.upper_bound( candidate.first );
  auto prevIter = pending_map.end();

  if ( pending_map.empty() == false && nextIter != pending_map.begin() ) {
    // 若nextIter为空，且pending_map非空，则向前取一元素，该元素一定存在
    prevIter = std::prev( nextIter );
  }
  // 这里使用了std::prev函数，它返回给定迭代器的前一个迭代器，而不改变原始迭代器。
  // 这是C++11标准引入的方法，用于在不改变原始迭代器的情况下获取前一个迭代器。

  if ( prevIter != pending_map.end() && end_idx( *prevIter ) >= candidate.first ) {
    // 若prevIter非空，并且与candidate交错
    auto end_of_prev = end_idx( *prevIter );
    if ( prevIter->first < candidate.first ) {
      // prevIter = nextIter--; // 只有当prevIter不是begin()时，且不与其相同，才能递减

      if ( end_of_prev >= candidate.first && end_of_prev < end_idx( candidate ) ) {
        // 有重叠部分，但不完全
        auto overlap_length = end_of_prev - candidate.first + 1;
        candidate.first = end_of_prev + 1;               // 更新该字符串的头部位置
        candidate.second.erase( 0, overlap_length );     // 删除重叠部分后的新数据
      } 
      else if ( end_of_prev >= end_idx( candidate ) ) { // !!!!!
        // 前一个元素的范围包含了当前插入的元素
        return; // 则直接返回
      }
    }
    // prevIter != pending_map.begin() == true说明prevIter前面还有元素
    // 现在prevIter指向的是candidate.first之前的元素，或者容器的第一个元素
    if ( prevIter->first == candidate.first ) {
      // auto end_of_candidate = candidate.first + candidate.second.size() - 1;
      // if ( end_of_prev > candidate.first ) {
      // 有重叠部分
      auto diff_idx = findFirstDifference( prevIter->second, candidate.second );
      if ( diff_idx != -1 ) {
        candidate.first = end_of_prev + 1;     // 更新该字符串的头部位置
        candidate.second.erase( 0, diff_idx ); // 删除重叠部分后的新数据
      }
    }
  }
  // 检查插入位置后的元素
  nextIter = pending_map.upper_bound( candidate.first );
  if ( nextIter != pending_map.end() ) {
    // 若nextIter非空，获取插入位置的后一个元素
    auto begin_of_next = nextIter->first;
    // auto end_of_next = end_idx( *nextIter );
    auto end_of_c = end_idx( candidate );
    // if ( end_of_c >= begin_of_next && end_of_c < end_of_next ) {
    if ( end_of_c >= begin_of_next) {
      // 该字符串与后一字符串的前部有重叠部分,但不完全覆盖
      // 删除该字符串的重叠部分
      auto overlap_length = end_of_c - begin_of_next + 1;
      std::string sub_string {};
      // sub_string = candidate.second.substr( candidate.second.size() - overlap_length, std::string::npos );

      try {
        sub_string = candidate.second.substr( candidate.second.size() - overlap_length, std::string::npos );
      } catch ( ... ) {
        std::cerr << candidate.first << std::endl << begin_of_next << std::endl;
        std::cerr << candidate.second.size() << std::endl << overlap_length << std::endl;
        throw std::runtime_error( "Unknown error occurred." );
      }
      auto sub_begin_idx = begin_of_next;
      candidate.second.erase( candidate.second.size() - overlap_length,
                              std::string::npos ); // 删除重叠部分后的新数据
      overlap_process( sub_begin_idx, sub_string, pending_map );
    } 
    // else if ( end_of_c >= end_of_next ) {
    //   // 该字符串包裹了后一个字符串
    //   pending_map.erase( nextIter ); // 删除后一个元素
    // }
  }
  uint64_t i = available_capacity - map_size;
  if ( i < candidate.second.size() ) {
    candidate.second = candidate.second.substr( 0, i );

    // try {
    //   candidate.second = candidate.second.substr( 0, i );
    // } catch ( ... ) {

    //   std::cerr << "Unknown error occurred." << std::endl;
    //   throw std::runtime_error( "Unknown error occurred." );
    // }
  }
  pending_map.insert( candidate );

  // return candidate;
}
// 处理
void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // 从0开始计数
  string this_data = std::move( data );
  if ( is_last_substring ) {
    // 若该子串是最后一个子串，则更新 eof_idx_
    has_last_ = true;
    if ( this_data.size() > 0 ) {
      eof_idx_ = first_index + this_data.size() - 1;
    } else {
      eof_idx_ = first_index - 1;
    }
  }
  if ( has_last_ && first_index > eof_idx_ && eof_idx_ != 0 && closed_ ) {
    // 若该子串的范围超出了最大范围，则直接返回  && eof_idx_ != 0
    return;
  }
  overlap_process( first_index, this_data, _pending_map );
  push_to_output();
  if ( ( first_unassembled_idx - 1 == eof_idx_ || current_end_idx == eof_idx_ ) && has_last_ ) {
    closed_ = true;
    output_.writer().close(); //&& eof_idx_ != 0  _next_assembled_idx - 1 == eof_idx_
  } //  || first_unassembled_idx == eof_idx_
}
void Reassembler::push_to_output()
{
  while ( true ) {
    auto iter = _pending_map.find( first_unassembled_idx );
    if ( iter == _pending_map.end() ) {
      break;
    }
    auto& data = iter->second;
    if ( output_.writer().available_capacity() < data.size() ) {
      break;
    }
    output_.writer().push( data );
    first_unassembled_idx += data.size();
    current_end_idx = first_unassembled_idx == 0 ? 0 : first_unassembled_idx - 1; // 处理空字符串并且结束的情况
    _pending_map.erase( iter );
  }
}
uint64_t Reassembler::bytes_pending() const
{
  // 计算待处理字节的总数。
  uint64_t count = 0;
  for ( const auto& pair : _pending_map ) {
    count += pair.second.size();
  }
  return count;
}
