
template <typename T, size_t size> 
class DoubleBuffer{
private:
	std::queue<T, size> buff_1, buff_2;
	std::queue<T, size> *read_buff, *write_buff;
	mutable std::mutex read;
	std::mutex write;
};