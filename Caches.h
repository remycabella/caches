#include <list>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <vector>

namespace Caches {

    template <typename K, typename V> class LRUCache {
        using ITERATOR = typename std::list <std::pair <K, V>>::iterator;
    
    public:
       
        LRUCache() = default;

        LRUCache(int size) : _maxSize(size) {}
        
        void produce(K key, V val) {
            std::lock_guard <std::mutex> guard(_mutex);
            _cacheList.emplace_front(std::pair <K, V>(key, val));
            auto it = m.find(key);
            if (it != m.end()) {
                _cacheList.erase(it->second);
                m.erase(it);
            }
            m[key] = _cacheList.begin();

            if (m.size() > _maxSize) {
                auto last = _cacheList.end();
                m.erase((--last)->first);    
                _cacheList.pop_back();
            }
        }
        
        V consume(K key) {
            std::lock_guard <std::mutex>  guard(_mutex);
            if (m.size() > 0) {
                auto it = m.find(key);
                if (it != m.end()) {
                    V val = it->second->second;
                    m.erase(it); 
                    _cacheList.erase(it->second);

                    return val;
                }
            }
            return V();
        }

        std::pair <K, V> consume() {
            std::lock_guard <std::mutex> guard(_mutex);
            if (m.size() > 0) {
                std::pair <K, V> content = _cacheList.back();
                m.erase(content.first);
                _cacheList.pop_back();
                return content;
            }
            return std::pair <K, V>(); 
        }

        std::vector <std::pair<K, V>> consume(int num) {
            std::lock_guard <std::mutex> guard(_mutex);
            std::vector <std::pair<K, V>> res;

            num = (m.size() > num) ? num : m.size();
            while (num-- > 0) {
                std::pair <K, V> content = _cacheList.back(); 
                m.erase(content.first);
                _cacheList.pop_back();

                res.emplace_back(content);                          
            }
            return res;
        }

        int size() {
            std::lock_guard <std::mutex> guard(_mutex);
            return m.size();
        }
        
        int capacity() {
            return _maxSize;
        }   

        void clear() {
            std::lock_guard <std::mutex> guard(_mutex);
            int size = _cacheList.size();
            while (size-- > 0) {
                _cacheList.pop_back();
            }

            m.clear();
        }
    

    private:
        std::list <std::pair <K, V>> _cacheList;

        std::unordered_map <K, ITERATOR> m;

        std::mutex _mutex;

        int _maxSize {1000};
    };

    template <typename T> class FIFOCache {
        using CONTENTYPE = typename std::vector <T>;

    public:
        FIFOCache() = default;

        FIFOCache(int size) : _maxSize(size) {}

        void produce(T t) {
            std::lock_guard <std::mutex> guard(_mutex);
            _cacheList.emplace_front(t);

            if (_cacheList.size() > _maxSize) {
                _cacheList.pop_back();    
            }
        }
       
        T consume() {
            std::lock_guard <std::mutex> guard(_mutex);
            if (_cacheList.size() > 0) {
                auto val = _cacheList.back();
                _cacheList.pop_back();
                return val;
            }
            return T();
        } 

        std::vector <T> consume(int num) {
            std::lock_guard <std::mutex> guard(_mutex);
            std::vector <T> res;
            num = (_cacheList.size() > num) ? num : _cacheList.size();
            
            while (num--  > 0) {
                auto val = _cacheList.back();
                _cacheList.pop_back();
                res.push_back(val);
            } 

            return res;
        }

        int size() {
            std::lock_guard <std::mutex> guard(_mutex);
            return _cacheList.size();
        }

        int capacity() {
            return _maxSize;
        }

        void clear() {
            std::lock_guard <std::mutex> guard(_mutex);
            int size = _cacheList.size();
            while (size-- > 0) {
                _cacheList.pop_back();
            }
        }
          
    private:
        std::list <T> _cacheList;   

        int _maxSize {1000};

        std::mutex _mutex;
    
    };

    template <typename K, typename V> class LFUCache {

    public:
        LFUCache() = default;
    
        LFUCache(int size) {
            _maxSize = size;
        }
    
        V consume(K key) {
            if (_cacheMap.find(key) != _cacheMap.end()) {
                _freqMap[_cacheMap[key].second].erase(_keyIter[key]);
                _freqMap[++(_cacheMap[key].second)].push_back(key);
                _keyIter[key] = --_freqMap[_cacheMap[key].second].end();
    
                if (_freqMap[_minFreq].size() == 0) {
                    _minFreq++;
                }
                return _cacheMap[key].first;
            } else {
                return V();
            }
        }
    
        void produce(K key, V value) {
            if (_maxSize <= 0) return;
    
            if (_cacheMap.count(key) != 0) {
                _freqMap[_cacheMap[key].second].erase(_keyIter[key]);
                _freqMap[++(_cacheMap[key].second)].push_back(key);
                _keyIter[key] = --_freqMap[_cacheMap[key].second].end();
                if (_freqMap[_minFreq].size() == 0) {
                    _minFreq++;
                }
                _cacheMap[key].first = value;
                return;
            }
            
            if (_cacheMap.size() >= _maxSize) {
                _cacheMap.erase(_freqMap[_minFreq].front());
                _keyIter.erase(_freqMap[_minFreq].front());
                _freqMap[_minFreq].pop_front();
            }
            _cacheMap[key] = {value, 1};
            _freqMap[1].push_back(key);
            _keyIter[key] = --_freqMap[1].end();
            _minFreq = 1;
        }

        int size() {
            std::lock_guard <std::mutex> guard(_mutex);
            return _cacheMap.size();
        }

        int capacity() {
            return _maxSize;
        }

        void clear() {
            std::lock_guard <std::mutex> guard(_mutex);
            _cacheMap.clear();
            _freqMap.clear();
            _keyIter.clear();
        }
    
    private:
        int _maxSize;
    
        int _minFreq;
    
        std::unordered_map<K, std::pair<V, int>> _cacheMap;
    
        std::unordered_map<int, std::list<K>> _freqMap;
    
        std::unordered_map<K, typename std::list<K>::iterator> _keyIter;
        
        std::mutex _mutex;
    };

};

