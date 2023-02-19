#ifndef _UNARY_TEST_H_INCLUDED_
#define _UNARY_TEST_H_INCLUDED_

#include <string>

void test_if_build_method_works_to_get_all_datas(const std::string &file_path, int mat_size, int queue_size, int buffer_size, double del1 = 1, double del2 = 1);
void test_if_filter_works_to_generate_filter_dictionnary(const std::string &file_path, int mat_size, int queue_size, int buffer_size, double del1 = 1, double del2 = 1);
void test_if_filter_then_unfilter_works(const std::string &file_path, int mat_size, int queue_size, int buffer_size, double del1 = 1, double del2 = 1);
void filter_and_save_filtered_file(const std::string &file_path, bool _2D, bool _3D, int mat_size, int queue_size, int buffer_size, double del1 = 1, double del2 = 1);
void test_if_unfilter_works_to_read_and_unfilters_datas(const std::string &file_path, int mat_size, int queue_size, int buffer_size, double del1 = 1, double del2 = 1);


#endif // _UNARY_TEST_H_INCLUDED_