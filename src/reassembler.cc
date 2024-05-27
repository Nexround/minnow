#include "reassembler.hh"
#include <iostream>
#include <string>
using namespace std;
std::pair<uint64_t, std::string> Reassembler::overlap_process( uint64_t first_index,
                                                               std::string data,
                                                               std::map<uint64_t, std::string>& pending_map )
{
  // index相同的情况？
  // 首先检查是否有重叠的部分
  std::pair<uint64_t, std::string> candidate = std::pair( first_index, data );
  // 对插入前后的元素进行检查
  // 原则：若要调整长度，则调整当前字符串

  // 获取插入位置前后的元素
  // 首先检查该字符串是否与前方字符串重叠
  // auto pos_iter = pending_map.upper_bound(first_index);
  // if (pos_iter != pending_map.begin())
  //   pos_iter--;
  if ( pending_map.lower_bound( candidate.first ) != pending_map.begin() ) { // 不是第一个元素
    // if ( candidate.first != pending_map.begin()->first ) { // 不是第一个元素
    // 获取前一个元素
    auto prevIter = pending_map.lower_bound( candidate.first );
    prevIter--; // 获取插入位置的前一个元素
    auto end_of_prev = prevIter->first + prevIter->second.size();
    if ( end_of_prev > candidate.first ) {
      // 有重叠部分
      auto overlap_length = end_of_prev - candidate.first;
      candidate.first = end_of_prev + 1;           // 更新该字符串的头部位置
      candidate.second.erase( 0, overlap_length ); // 删除重叠部分后的新数据
      // _pending_map.emplace( end_of_prev + 1, std::move( new_data ) );    // 插入新的元素
    } else if ( end_of_prev > candidate.first ) {
      // 前一个元素的范围包含了当前插入的元素
      return std::pair<uint64_t, std::string>(); // 则直接返回
    }
  }
  // 检查插入位置后的元素
  auto nextIter = pending_map.upper_bound( candidate.first );
  if ( nextIter != pending_map.end() ) {
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
      pending_map.erase( nextIter ); // 删除后一个元素
    }
  }
  pending_map.insert( candidate );

  return candidate;
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
      eof_idx_ = first_index;
    }
  }
  if ( has_last_ && first_index + this_data.size() > eof_idx_ && eof_idx_ != 0 ) {
    // 若该子串的范围超出了最大范围，则直接返回  && eof_idx_ != 0
    return;
  }
  std::pair<uint64_t, std::string> candidate = overlap_process( first_index, this_data, _pending_map );
  // 若 first_index == _next_assembled_idx，直接写入
  // uint64_t available_capacity = output_.writer().available_capacity();
  if ( candidate.first == _next_assembled_idx && candidate.second.size() > 0 ) {
    output_.writer().push( candidate.second );
    _pending_map.erase( candidate.first );          // 删除该元素
    _next_assembled_idx += candidate.second.size(); // 更新下一个要写入的位置
    // _next_assembled_idx = current_end_idx + 1;
  }
  if ( ( _next_assembled_idx - 1 == eof_idx_ || _next_assembled_idx == eof_idx_ ) && has_last_ ) {
    output_.writer().close(); //&& eof_idx_ != 0  _next_assembled_idx - 1 == eof_idx_
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
