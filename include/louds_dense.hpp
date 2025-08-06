#ifndef LOUDSDENSE_H_
#define LOUDSDENSE_H_

#include <string>

#include "config.hpp"
#include "rank.hpp"
#include "suffix.hpp"
#include "surf_builder.hpp"

namespace surf {

class LoudsDense {
public:
    class Iter {
    public:
	Iter() : is_valid_(false) {}
	Iter(LoudsDense* trie) : is_valid_(false), is_search_complete_(false),
				 is_move_left_complete_(false),
				 is_move_right_complete_(false),
				 trie_(trie),
				 send_out_node_num_(0), key_len_(0),
				 is_at_prefix_key_(false) {
	    for (level_t level = 0; level < trie_->getHeight(); level++) {
		key_.push_back(0);
		pos_in_trie_.push_back(0);
	    }
	}

	void clear();
	bool isValid() const { return is_valid_; }
	bool isSearchComplete() const { return is_search_complete_; }
	bool isMoveLeftComplete() const { return is_move_left_complete_; }
	bool isMoveRightComplete() const { return is_move_right_complete_; }
	bool isComplete() const {
	    return (is_search_complete_ &&
		    (is_move_left_complete_ && is_move_right_complete_));
	}

	int compare(const std::string& key) const;
	std::string getKey() const;
	int getSuffix(word_t* suffix) const;
	std::string getKeyWithSuffix(unsigned* bitlen) const;
	position_t getSendOutNodeNum() const { return send_out_node_num_; }

	void setToFirstLabelInRoot();
	void setToLastLabelInRoot();
	void moveToLeftMostKey();
	void moveToRightMostKey();
	void operator ++(int);
	void operator --(int);

    private:
	inline void append(position_t pos);
	inline void set(level_t level, position_t pos);
	inline void setSendOutNodeNum(position_t node_num) { send_out_node_num_ = node_num; }
	inline void setFlags(const bool is_valid, const bool is_search_complete, 
			     const bool is_move_left_complete,
			     const bool is_move_right_complete);

    private:
	// True means the iter either points to a valid key 
	// or to a prefix with length trie_->getHeight()
	bool is_valid_;
	// If false, call moveToKeyGreaterThan in LoudsSparse to complete
	bool is_search_complete_; 
	// If false, call moveToLeftMostKey in LoudsSparse to complete
	bool is_move_left_complete_;
	// If false, call moveToRightMostKey in LoudsSparse to complete
	bool is_move_right_complete_; 
	LoudsDense* trie_;
	position_t send_out_node_num_;
	level_t key_len_; // Does NOT include suffix

	std::vector<label_t> key_;
	std::vector<position_t> pos_in_trie_;
	bool is_at_prefix_key_;

	friend class LoudsDense;
    };

public:
    LoudsDense() {}
    LoudsDense(const SuRFBuilder* builder);

    ~LoudsDense() {}

    // Returns whether key exists in the trie so far
    // out_node_num == 0 means search terminates in louds-dense.
    bool lookupKey(const std::string& key, position_t& out_node_num) const;
    // return value indicates potential false positive
    bool moveToKeyGreaterThan(const std::string& key, 
			      const bool inclusive, LoudsDense::Iter& iter) const;
    uint64_t approxCount(const LoudsDense::Iter* iter_left,
			 const LoudsDense::Iter* iter_right,
			 position_t& out_node_num_left,
			 position_t& out_node_num_right) const;

    uint64_t getHeight() const { return height_; }
    uint64_t serializedSize() const;
    uint64_t getMemoryUsage() const;

    void serialize(char*& dst) const {
	memcpy(dst, &height_, sizeof(height_));
	dst += sizeof(height_);
	memcpy(dst, level_cuts_, sizeof(position_t) * height_);
	dst += (sizeof(position_t) * height_);
	align(dst);
	label_bitmaps_->serialize(dst);
	child_indicator_bitmaps_->serialize(dst);
	prefixkey_indicator_bits_->serialize(dst);
	suffixes_->serialize(dst);
	align(dst);
    }

    static LoudsDense* deSerialize(char*& src) {
	LoudsDense* louds_dense = new LoudsDense();
	memcpy(&(louds_dense->height_), src, sizeof(louds_dense->height_));
	src += sizeof(louds_dense->height_);
	louds_dense->level_cuts_ = new position_t[louds_dense->height_];
	memcpy(louds_dense->level_cuts_, src,
	       sizeof(position_t) * (louds_dense->height_));
	src += (sizeof(position_t) * (louds_dense->height_));
	align(src);
	louds_dense->label_bitmaps_ = BitvectorRank::deSerialize(src);
	louds_dense->child_indicator_bitmaps_ = BitvectorRank::deSerialize(src);
	louds_dense->prefixkey_indicator_bits_ = BitvectorRank::deSerialize(src);
	louds_dense->suffixes_ = BitvectorSuffix::deSerialize(src);
	align(src);
	return louds_dense;
    }

    void destroy() {
	label_bitmaps_->destroy();
	child_indicator_bitmaps_->destroy();
	prefixkey_indicator_bits_->destroy();
	suffixes_->destroy();
    }

private:
    position_t getChildNodeNum(const position_t pos) const;
    position_t getSuffixPos(const position_t pos, const bool is_prefix_key) const;
    position_t getNextPos(const position_t pos) const;
    position_t getPrevPos(const position_t pos, bool* is_out_of_bound) const;

	bool compareSuffixGreaterThan(const position_t pos, const std::string& key, 
				  const level_t level, 
				  LoudsDense::Iter& iter) const;
    void extendPosList(std::vector<position_t>& pos_list,
		       position_t& out_node_num) const;

private:
    static const position_t kNodeFanout = 256;
    static const position_t kRankBasicBlockSize  = 512;

    level_t height_;
    position_t* level_cuts_; // position of the last bit at each level

    BitvectorRank* label_bitmaps_;
    BitvectorRank* child_indicator_bitmaps_;
    BitvectorRank* prefixkey_indicator_bits_; //1 bit per internal node
    BitvectorSuffix* suffixes_;
};


LoudsDense::LoudsDense(const SuRFBuilder* builder) {
    height_ = builder->getSparseStartLevel();
    std::vector<position_t> num_bits_per_level;
    for (level_t level = 0; level < height_; level++)
	num_bits_per_level.push_back(static_cast<position_t>(builder->getBitmapLabels()[level].size()) * kWordSize);

    level_cuts_ = new position_t[height_];
    position_t bit_count = 0;
    for (level_t level = 0; level < height_; level++) {
	bit_count += num_bits_per_level[level];
	level_cuts_[level] = bit_count - 1;
    }

    label_bitmaps_ = new BitvectorRank(kRankBasicBlockSize, builder->getBitmapLabels(),
				       num_bits_per_level, 0, height_);
    child_indicator_bitmaps_ = new BitvectorRank(kRankBasicBlockSize,
						 builder->getBitmapChildIndicatorBits(),
						 num_bits_per_level, 0, height_);
    prefixkey_indicator_bits_ = new BitvectorRank(kRankBasicBlockSize,
						  builder->getPrefixkeyIndicatorBits(),
						  builder->getNodeCounts(), 0, height_);

    if (builder->getSuffixType() == kNone) {
	suffixes_ = new BitvectorSuffix();
    } else {
	level_t hash_suffix_len = builder->getHashSuffixLen();
        level_t real_suffix_len = builder->getRealSuffixLen();
        level_t suffix_len = hash_suffix_len + real_suffix_len;
	std::vector<position_t> num_suffix_bits_per_level;
	for (level_t level = 0; level < height_; level++)
	    num_suffix_bits_per_level.push_back(builder->getSuffixCounts()[level] * suffix_len);
	suffixes_ = new BitvectorSuffix(builder->getSuffixType(), 
					hash_suffix_len, real_suffix_len,
                                        builder->getSuffixes(),
					num_suffix_bits_per_level, 0, height_);
    }
}

bool LoudsDense::lookupKey(const std::string& key, position_t& out_node_num) const {
    position_t node_num = 0;
    position_t pos = 0;
    for (level_t level = 0; level < height_; level++) {
	pos = (node_num * kNodeFanout);
	if (level >= key.length()) { //if run out of searchKey bytes
	    if (prefixkey_indicator_bits_->readBit(node_num)) //if the prefix is also a key
		return suffixes_->checkEquality(getSuffixPos(pos, true), key, level + 1);
	    else
		return false;
	}
	pos += static_cast<label_t>(key[level]);

	//child_indicator_bitmaps_->prefetch(pos);

	if (!label_bitmaps_->readBit(pos)) //if key byte does not exist
	    return false;

	if (!child_indicator_bitmaps_->readBit(pos)) //if trie branch terminates
	    return suffixes_->checkEquality(getSuffixPos(pos, false), key, level + 1);

	node_num = getChildNodeNum(pos);
    }
    //search will continue in LoudsSparse
    out_node_num = node_num;
    return true;
}

bool LoudsDense::moveToKeyGreaterThan(const std::string& key, 
					  const bool inclusive, LoudsDense::Iter& iter) const {
	(void)inclusive;
	position_t node_num = 0;
	position_t pos = 0;
	for (level_t level = 0; level < height_; level++) {
	// if is_at_prefix_key_, pos is at the next valid position in the child node
	pos = node_num * kNodeFanout;
	if (level >= key.length()) { // if run out of searchKey bytes
	    iter.append(getNextPos(pos - 1));
	    if (prefixkey_indicator_bits_->readBit(node_num)) //if the prefix is also a key
		iter.is_at_prefix_key_ = true;
	    else
		iter.moveToLeftMostKey();
	    // valid, search complete, moveLeft complete, moveRight complete
	    iter.setFlags(true, true, true, true); 
	    return true;
	}

	pos += static_cast<label_t>(key[level]);
	iter.append(pos);

	// if no exact match
	if (!label_bitmaps_->readBit(pos)) {
	    iter++;
	    return false;
	}
	//if trie branch terminates
	if (!child_indicator_bitmaps_->readBit(pos))
		return compareSuffixGreaterThan(pos, key, level+1, iter);
	node_num = getChildNodeNum(pos);
    }

    //search will continue in LoudsSparse
    iter.setSendOutNodeNum(node_num);
    // valid, search INCOMPLETE, moveLeft complete, moveRight complete
    iter.setFlags(true, false, true, true);
    return true;
}

void LoudsDense::extendPosList(std::vector<position_t>& pos_list,
			       position_t& out_node_num) const {
    position_t node_num = 0;
    position_t pos = pos_list[pos_list.size() - 1];
	for (level_t i = static_cast<level_t>(pos_list.size()); i < height_; i++) {
	node_num = getChildNodeNum(pos);
	if (!child_indicator_bitmaps_->readBit(pos))
	    node_num++;
	pos = (node_num * kNodeFanout);
	if (pos > level_cuts_[i]) {
	    pos = kMaxPos;
	    pos_list.push_back(pos);
	    break;
	}
	pos_list.push_back(pos);
    }
    if (pos == kMaxPos) {
	for (level_t i = static_cast<level_t>(pos_list.size()); i < height_; i++)
		pos_list.push_back(pos);
	out_node_num = pos;
    } else {
	out_node_num = getChildNodeNum(pos);
	if (!child_indicator_bitmaps_->readBit(pos))
	    out_node_num++;
    }
}

uint64_t LoudsDense::approxCount(const LoudsDense::Iter* iter_left,
				 const LoudsDense::Iter* iter_right,
				 position_t& out_node_num_left,
				 position_t& out_node_num_right) const {
    std::vector<position_t> left_pos_list, right_pos_list;
	for (level_t i = 0; i < iter_left->key_len_; i++){
		left_pos_list.push_back(iter_left->pos_in_trie_[i]);
	}
	level_t ori_left_len = static_cast<level_t>(left_pos_list.size());
	extendPosList(left_pos_list, out_node_num_left);

    for (level_t i = 0; i < iter_right->key_len_; i++){
		right_pos_list.push_back(iter_right->pos_in_trie_[i]);
	}
	level_t ori_right_len = static_cast<level_t>(right_pos_list.size());
    extendPosList(right_pos_list, out_node_num_right);

    uint64_t count = 0;
    for (level_t i = 0; i < height_; i++) {
	position_t left_pos = left_pos_list[i];
	if (left_pos == kMaxPos) break;
	if (i == (ori_left_len - 1) && iter_left->is_at_prefix_key_)
	    left_pos = (left_pos / kNodeFanout) * kNodeFanout;
	position_t right_pos = right_pos_list[i];
	if (right_pos == kMaxPos)
	    right_pos = level_cuts_[i];
	if (i == (ori_right_len - 1) && iter_right->is_at_prefix_key_)
	    right_pos = (right_pos / kNodeFanout) * kNodeFanout;
	//assert(left_pos <= right_pos);
	if (left_pos < right_pos) {
	    if (i >= ori_left_len)
		left_pos = getNextPos(left_pos);
	    if (i >= ori_right_len && right_pos != level_cuts_[height_ - 1])
		right_pos = getNextPos(right_pos);
	    bool has_prefix_key_left
		= prefixkey_indicator_bits_->readBit(left_pos / kNodeFanout);
	    bool has_prefix_key_right
		= prefixkey_indicator_bits_->readBit(right_pos / kNodeFanout);
	    position_t rank_left_label = label_bitmaps_->rank(left_pos);
	    position_t rank_right_label = label_bitmaps_->rank(right_pos);
	    if (right_pos == level_cuts_[height_ - 1])
		rank_right_label++;
	    position_t rank_left_ind = child_indicator_bitmaps_->rank(left_pos);
	    position_t rank_right_ind = child_indicator_bitmaps_->rank(right_pos);
	    position_t rank_left_prefix
		= prefixkey_indicator_bits_->rank(left_pos / kNodeFanout);
	    position_t rank_right_prefix
		= prefixkey_indicator_bits_->rank(right_pos / kNodeFanout);
	    position_t num_leafs = (rank_right_label - rank_left_label)
		- (rank_right_ind - rank_left_ind)
		+ (rank_right_prefix - rank_left_prefix);
	    // offcount in child_indicators
	    if (child_indicator_bitmaps_->readBit(right_pos))
		num_leafs++;
	    if (child_indicator_bitmaps_->readBit(left_pos))
		num_leafs--;
	    // offcount in prefix keys
	    if (i >= ori_right_len && has_prefix_key_right)
		num_leafs--;
	    if (i >= ori_left_len && has_prefix_key_left)
		num_leafs++;
	    if (iter_left->is_search_complete_ && (i == ori_left_len - 1))
		num_leafs--;
	    count += num_leafs;
	}
    }
    return count;
}

uint64_t LoudsDense::serializedSize() const {
    uint64_t size = sizeof(height_)
	+ (sizeof(position_t) * height_);
    sizeAlign(size);
    size += (label_bitmaps_->serializedSize()
	     + child_indicator_bitmaps_->serializedSize()
	     + prefixkey_indicator_bits_->serializedSize()
	     + suffixes_->serializedSize());
    sizeAlign(size);
    return size;
}

uint64_t LoudsDense::getMemoryUsage() const {
    return (sizeof(LoudsDense)
	    + label_bitmaps_->size()
	    + child_indicator_bitmaps_->size()
	    + prefixkey_indicator_bits_->size()
	    + suffixes_->size());
}

position_t LoudsDense::getChildNodeNum(const position_t pos) const {
    return child_indicator_bitmaps_->rank(pos);
}

position_t LoudsDense::getSuffixPos(const position_t pos, const bool is_prefix_key) const {
    position_t node_num = pos / kNodeFanout;
    position_t suffix_pos = (label_bitmaps_->rank(pos)
			     - child_indicator_bitmaps_->rank(pos)
			     + prefixkey_indicator_bits_->rank(node_num)
			     - 1);
    if (is_prefix_key && label_bitmaps_->readBit(pos) && !child_indicator_bitmaps_->readBit(pos))
	suffix_pos--;
    return suffix_pos;
}

position_t LoudsDense::getNextPos(const position_t pos) const {
    return pos + label_bitmaps_->distanceToNextSetBit(pos);
}

position_t LoudsDense::getPrevPos(const position_t pos, bool* is_out_of_bound) const {
    position_t distance = label_bitmaps_->distanceToPrevSetBit(pos);
    if (pos <= distance) {
	*is_out_of_bound = true;
	return 0;
    }
    *is_out_of_bound = false;
    return (pos - distance);
}

bool LoudsDense::compareSuffixGreaterThan(const position_t pos, const std::string& key, 
					  const level_t level, 
					  LoudsDense::Iter& iter) const {
	position_t suffix_pos = getSuffixPos(pos, false);
	int compare = suffixes_->compare(suffix_pos, key, level);
	if ((compare != kCouldBePositive) && (compare < 0)) {
	iter++;
	return false;
	}
	// valid, search complete, moveLeft complete, moveRight complete
	iter.setFlags(true, true, true, true);
	return true;
}

//============================================================================

void LoudsDense::Iter::clear() {
    is_valid_ = false;
    key_len_ = 0;
    is_at_prefix_key_ = false;
}

int LoudsDense::Iter::compare(const std::string& key) const {
    if (is_at_prefix_key_ && (key_len_ - 1) < key.length())
	return -1;
    std::string iter_key = getKey();
    std::string key_dense = key.substr(0, iter_key.length());
    int compare = iter_key.compare(key_dense);
    if (compare != 0) return compare;
    if (isComplete()) {
		position_t suffix_pos = trie_->getSuffixPos(pos_in_trie_[key_len_ - 1], is_at_prefix_key_);
		return trie_->suffixes_->compare(suffix_pos, key, key_len_);
    }
    return compare;
}

std::string LoudsDense::Iter::getKey() const {
    if (!is_valid_){
		return std::string();
	}
    level_t len = key_len_;
    if (is_at_prefix_key_){
		len--;
	}
	return std::string(reinterpret_cast<const char*>(key_.data()), static_cast<size_t>(len));
}

int LoudsDense::Iter::getSuffix(word_t* suffix) const {
    if (isComplete()
        && ((trie_->suffixes_->getType() == kReal) || (trie_->suffixes_->getType() == kMixed))) {
	position_t suffix_pos = trie_->getSuffixPos(pos_in_trie_[key_len_ - 1], is_at_prefix_key_);
	*suffix = trie_->suffixes_->readReal(suffix_pos);
	return trie_->suffixes_->getRealSuffixLen();
    }
    *suffix = 0;
    return 0;
}

std::string LoudsDense::Iter::getKeyWithSuffix(unsigned* bitlen) const {
    std::string iter_key = getKey();
    if (isComplete()
        && ((trie_->suffixes_->getType() == kReal) || (trie_->suffixes_->getType() == kMixed))) {
	position_t suffix_pos = trie_->getSuffixPos(pos_in_trie_[key_len_ - 1], is_at_prefix_key_);
	word_t suffix = trie_->suffixes_->readReal(suffix_pos);
	if (suffix > 0) {
	    level_t suffix_len = trie_->suffixes_->getRealSuffixLen();
	    *bitlen = suffix_len % 8;
	    suffix <<= (64 - suffix_len);
	    char* suffix_str = reinterpret_cast<char*>(&suffix);
	    suffix_str += 7;
	    unsigned pos = 0;
	    while (pos < suffix_len) {
		iter_key.append(suffix_str, 1);
		suffix_str--;
		pos += 8;
	    }
	}
    }
    return iter_key;
}

void LoudsDense::Iter::append(position_t pos) {
    assert(key_len_ < key_.size());
	key_[key_len_] = static_cast<label_t>(pos % kNodeFanout);
    pos_in_trie_[key_len_] = pos;
    key_len_++;
}

void LoudsDense::Iter::set(level_t level, position_t pos) {
    assert(level < key_.size());
	key_[level] = static_cast<label_t>(pos % kNodeFanout);
    pos_in_trie_[level] = pos;
}

void LoudsDense::Iter::setFlags(const bool is_valid,
				const bool is_search_complete, 
				const bool is_move_left_complete,
				const bool is_move_right_complete) {
    is_valid_ = is_valid;
    is_search_complete_ = is_search_complete;
    is_move_left_complete_ = is_move_left_complete;
    is_move_right_complete_ = is_move_right_complete;
}

void LoudsDense::Iter::setToFirstLabelInRoot() {
    if (trie_->label_bitmaps_->readBit(0)) {
	pos_in_trie_[0] = 0;
	key_[0] = static_cast<label_t>(0);
    } else {
	pos_in_trie_[0] = trie_->getNextPos(0);
	key_[0] = static_cast<label_t>(pos_in_trie_[0]);
    }
    key_len_++;
}

void LoudsDense::Iter::setToLastLabelInRoot() {
    bool is_out_of_bound;
    pos_in_trie_[0] = trie_->getPrevPos(kNodeFanout, &is_out_of_bound);
	key_[0] = static_cast<label_t>(pos_in_trie_[0]);
    key_len_++;
}

void LoudsDense::Iter::moveToLeftMostKey() {
    assert(key_len_ > 0);
    level_t level = key_len_ - 1;
    position_t pos = pos_in_trie_[level];
    if (!trie_->child_indicator_bitmaps_->readBit(pos))
	// valid, search complete, moveLeft complete, moveRight complete
	return setFlags(true, true, true, true);

    while (level < trie_->getHeight() - 1) {
	position_t node_num = trie_->getChildNodeNum(pos);
	//if the current prefix is also a key
	if (trie_->prefixkey_indicator_bits_->readBit(node_num)) {
	    append(trie_->getNextPos(node_num * kNodeFanout - 1));
	    is_at_prefix_key_ = true;
	    // valid, search complete, moveLeft complete, moveRight complete
	    return setFlags(true, true, true, true);
	}

	pos = trie_->getNextPos(node_num * kNodeFanout - 1);
	append(pos);

	// if trie branch terminates
	if (!trie_->child_indicator_bitmaps_->readBit(pos))
	    // valid, search complete, moveLeft complete, moveRight complete
	    return setFlags(true, true, true, true);

	level++;
    }
    send_out_node_num_ = trie_->getChildNodeNum(pos);
    // valid, search complete, moveLeft INCOMPLETE, moveRight complete
    setFlags(true, true, false, true);
}

void LoudsDense::Iter::moveToRightMostKey() {
    assert(key_len_ > 0);
    level_t level = key_len_ - 1;
    position_t pos = pos_in_trie_[level];
    if (!trie_->child_indicator_bitmaps_->readBit(pos))
	// valid, search complete, moveLeft complete, moveRight complete
	return setFlags(true, true, true, true);

    while (level < trie_->getHeight() - 1) {
	position_t node_num = trie_->getChildNodeNum(pos);
	bool is_out_of_bound;
	pos = trie_->getPrevPos((node_num + 1) * kNodeFanout, &is_out_of_bound);
	if (is_out_of_bound) {
	    is_valid_ = false;
	    return;
	}
	append(pos);

	// if trie branch terminates
	if (!trie_->child_indicator_bitmaps_->readBit(pos))
	    // valid, search complete, moveLeft complete, moveRight complete
	    return setFlags(true, true, true, true);

	level++;
    }
    send_out_node_num_ = trie_->getChildNodeNum(pos);
    // valid, search complete, moveleft complete, moveRight INCOMPLETE
    setFlags(true, true, true, false);
}

void LoudsDense::Iter::operator ++(int) {
    assert(key_len_ > 0);
    if (is_at_prefix_key_) {
	is_at_prefix_key_ = false;
	return moveToLeftMostKey();
    }
    position_t pos = pos_in_trie_[key_len_ - 1];
    position_t next_pos = trie_->getNextPos(pos);
    // if crossing node boundary
    while ((next_pos / kNodeFanout) > (pos / kNodeFanout)) {
	key_len_--;
	if (key_len_ == 0) {
	    is_valid_ = false;
	    return;
	}
	pos = pos_in_trie_[key_len_ - 1];
	next_pos = trie_->getNextPos(pos);
    }
    set(key_len_ - 1, next_pos);
    return moveToLeftMostKey();
}

void LoudsDense::Iter::operator --(int) {
    assert(key_len_ > 0);
    if (is_at_prefix_key_) {
	is_at_prefix_key_ = false;
	key_len_--;
    }
    position_t pos = pos_in_trie_[key_len_ - 1];
    bool is_out_of_bound;
    position_t prev_pos = trie_->getPrevPos(pos, &is_out_of_bound);
    if (is_out_of_bound) {
	is_valid_ = false;
	return;
    }
    
    // if crossing node boundary
    while ((prev_pos / kNodeFanout) < (pos / kNodeFanout)) {
	//if the current prefix is also a key
	position_t node_num = pos / kNodeFanout;
	if (trie_->prefixkey_indicator_bits_->readBit(node_num)) {
	    is_at_prefix_key_ = true;
	    // valid, search complete, moveLeft complete, moveRight complete
	    return setFlags(true, true, true, true);
	}
	
	key_len_--;
	if (key_len_ == 0) {
	    is_valid_ = false;
	    return;
	}
	pos = pos_in_trie_[key_len_ - 1];
	prev_pos = trie_->getPrevPos(pos, &is_out_of_bound);
	if (is_out_of_bound) {
	    is_valid_ = false;
	    return;
	}
    }
    set(key_len_ - 1, prev_pos);
    return moveToRightMostKey();
}

} //namespace surf

#endif // LOUDSDENSE_H_
