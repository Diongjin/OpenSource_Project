#include <numeric>
#include <iostream>
#include <iterator>
#include <vector>
#include <algorithm>

int main()
{
	std::vector<int> data;

	data.push_back(2);
	data.push_back(4);
	data.push_back(6);
	data.push_back(5);
	data.push_back(8);
	data.push_back(7);

	std::cout<<"배열 용량은 "<< data.capacity()<<std::endl;

	data.erase(std::begin(data) + 2);
	std::cout <<"배열 원소 : ";
	for (auto iter = std::begin(data); iter != std::end(data);
++iter)
		std::cout << *iter << " ";
	std::cout << std::endl;

	std::sort(std::begin(data), std::end(data));
	std::cout << "배열 원소: ";
	for (auto iter = std::rbegin(data); iter != std::rend(data);
++iter)
		std::cout << *iter << " ";
	std::cout << std::endl;
}
