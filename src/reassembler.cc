#include "reassembler.hh"
#include <iostream>
using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // cout << is_last_substring;
  string this_data = std::move( data );
  if ( is_last_substring ) {
    // 若该子串是最后一个子串，则更新 eof_idx_
    eof_idx_ = first_index + this_data.size();
  }
  if ( ( first_index >= eof_idx_ || first_index + this_data.size() > eof_idx_ ) && eof_idx_ != 0 ) {
    // 若该子串的范围超出了最大范围，则直接返回
    return;
  }

  // index相同的情况？
  // 首先检查是否有重叠的部分
  auto candidate = std::pair( first_index, this_data );
  // 对插入前后的元素进行检查
  // 原则：若要调整长度，则调整当前字符串

  // 获取插入位置前后的元素
  // 首先检查该字符串是否与前方字符串重叠
  if ( candidate.first != _pending_map.begin()->first ) { // 不是第一个元素
    // 获取前一个元素
    auto prevIter = _pending_map.lower_bound( candidate.first );
    prevIter--; // 获取插入位置的前一个元素
    auto end_of_prev = prevIter->first + prevIter->second.size();
    if ( end_of_prev > candidate.first ) {
      // 有重叠部分
      auto overlap_length = end_of_prev - candidate.first;
      candidate.second.erase( 0, overlap_length ); // 删除重叠部分后的新数据
      // _pending_map.emplace( end_of_prev + 1, std::move( new_data ) );    // 插入新的元素
    } else if ( end_of_prev > candidate.first ) {
      // 前一个元素的范围包含了当前插入的元素
      return; // 则直接返回
    }
  }
  // 检查插入位置后的元素
  auto nextIter = _pending_map.upper_bound( candidate.first );
  if ( nextIter != _pending_map.end() ) {
    // 若nextIter非空，获取插入位置的后一个元素
    auto begin_of_next = nextIter->first;
    auto end_of_next = nextIter->first + nextIter->second.size();
    if ( candidate.first + candidate.second.size() > begin_of_next ) {
      // 该字符串与后一字符串的前部有重叠部分
      // 删除该字符串的重叠部分
      auto overlap_length = candidate.first + candidate.second.size() - begin_of_next;
      candidate.second.erase( candidate.second.size() - overlap_length, overlap_length ); // 删除重叠部分后的新数据
      // _pending_map.emplace( end_of_prev + 1, std::move( new_data ) );    // 插入新的元素
    } else if ( candidate.first + candidate.second.size() > end_of_next ) {
      // 该字符串包裹了后一个字符串
      _pending_map.erase( nextIter ); // 删除后一个元素
    }
  }
  _pending_map.insert( candidate );
  // 若 first_index == _next_assembled_idx，直接写入
  if ( first_index == _next_assembled_idx ) {
    output_.writer().push( candidate.second );
    _next_assembled_idx += candidate.second.size() + 1; // 更新下一个要写入的位置
  }
  if ( _next_assembled_idx - 1 == eof_idx_ && eof_idx_ != (unsigned long)( 0 - 1 ) ) {
    output_.writer().close();
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
