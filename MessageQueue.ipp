template <typename T>
MessageQueue<T>::MessageQueue() {
  // you may want to changes this
}

template <typename T>
bool MessageQueue<T>::Add(const T& val) {
  return false;  // you may want to change this
}

template <typename T>
void MessageQueue<T>::Close() {
  // you may want to changes this
}

template <typename T>
std::optional<T> MessageQueue<T>::Remove() {
  return std::nullopt;  // you may want to changes this
}

template <typename T>
std::optional<T> MessageQueue<T>::WaitRemove() {
  return std::nullopt;  // you may want to change this
}

template <typename T>
int MessageQueue<T>::Size() {
  return -1;  // you may want to change this
}
