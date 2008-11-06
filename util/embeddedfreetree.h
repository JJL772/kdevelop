/* This file is part of KDevelop
    Copyright 2008 David Nolden <david.nolden.kdevelop@art-master.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef EMBEDDED_FREE_TREE
#define EMBEDDED_FREE_TREE

#include <sys/types.h>
#include <limits>
#include <stdlib.h>
#include <QPair>

//Uncomment this to search for tree-inconsistencies, however it's very expensive
// #define DEBUG_FREEITEM_COUNT debugFreeItemCount(); verifyTreeConsistent(*m_centralFreeItem, 0, m_itemCount);
#define DEBUG_FREEITEM_COUNT

/**
 * This file implements algorithms that allow managing a sorted list of items, and managing "free" items
 * for reuse efficiently in that list. Among those free items a tree is built, and they are traversed
 * on insertion/removal to manage free items in the tree.
 *
 * There is specific needs on the embedded items:
 * - They must be markable "invalid", so after they are deleted they can stay in their place as invalid items.
 * - While they are invalid, they still must be able to hold 2 integers, needed for managing the tree of free items.
 * - One integer is needed for each list to hold a pointer to the central free item.
 * 
 * Only these functions must be used to manipulate the lists, from the beginning up. First create an empty list
 * and initialize centralFreeItem with -1, and then you start adding items.
 * 
 * Since the list is sorted, and each item can be contained only once, these lists actually represent a set.
 * 
 * EmbeddedTreeAlgorithms implements an efficient "contains" function that uses binary search within the list.
 */

namespace KDevelop {
    ///@todo Better dynamic rebalancing the tree
    
    ///Responsible for handling the items in the list
    ///This is an example. ItemHandler::rightChild(..) and ItemHandler::leftChild(..) must be values that must be able to hold the count of positive
    ///values that will be the maximum size of the list, and additionally -1.
//     template<class Data>
//     class ExampleItemHandler {
//         public:
//         ExampleItemHandler(const Data& data) : m_data(data) {
//         }
//         int ItemHandler::rightChild() const {
//             Q_ASSERT(0);
//         }
//         int ItemHandler::leftChild() const {
//             Q_ASSERT(0);
//         }
//         void ItemHandler::setLeftChild(int child) {
//             Q_ASSERT(0);
//         }
//         void setRightChild(int child) {
//             Q_ASSERT(0);
//         }
//         bool operator<(const StandardItemHandler& rhs) const {
//             Q_ASSERT(0);
//         }
//         //Copies this item into the given one
//         void copyTo(Data& data) const {
//             data = m_data;
//         }
//         
//         static void createFreeItem(Data& data) {
//             data = Data();
//         }
//         
//         bool isFree() const {
//             Q_ASSERT(0);
//         }
//
//         const Data& data() {
//         }
//         
//         private:
//             const Data& m_data;
//     };

    /**
     * Use this for several constant algorithms on sorted lists with free-trees
     * */
    template<class Data, class ItemHandler>
    class EmbeddedTreeAlgorithms {

        public:
            
            EmbeddedTreeAlgorithms(const Data* items, uint itemCount, const int& centralFreeItem) : m_items(items), m_itemCount(itemCount), m_centralFreeItem(&centralFreeItem) {
            }
            ~EmbeddedTreeAlgorithms() {
            }

            ///Efficiently checks whether the item is contained in the set.
            ///If it is contained, returns the index. Else, returns -1.

            int indexOf(const Data& data) {
                return indexOf(data, 0, m_itemCount);
            }

            ///Searches the given item within the specified bounds.
            int indexOf(const Data& data, uint start, uint end) {
                while(1) {
                    if(start == end)
                        return -1;
                    if(ItemHandler::equals(m_items[start], data))
                        return start;
                    if(end-start == 1) //The range only consists of 'start'
                        return -1;
                    
                    //Skip free items, since they cannot be used for ordering
                    uint center = (start + end)/2;
                    for(; center < end; ) {
                        if(!ItemHandler::isFree(m_items[center]))
                            break;
                        ++center;
                    }
                    if(center == end) {
                        end = (start + end)/2; //No non-free items found in second half, so continue search in the other
                    }else{
                        if(data < m_items[center]) {
                            end = (start + end)/2;
                        }else{
                            //Continue search in second half
                            start = center;
                        }
                    }
                }
                return -1;
            }

            ///Returns the first valid index that has a data-value larger or equal to @param data.
            ///Returns -1 if nothing is found.
            int lowerBound(const Data& data, uint start, uint end) {
                int currentBound = -1;
                while(1) {
                    if(start == end)
                        return currentBound;
                    if(ItemHandler::equals(m_items[start], data))
                        return start;
                    if(end-start == 1) { //The range only consists of 'start'
                        if(data < m_items[start])
                            return start;
                        else
                            return currentBound;
                    }
                    
                    //Skip free items, since they cannot be used for ordering
                    uint center = (start + end)/2;
                    for(; center < end; ) {
                        if(!ItemHandler::isFree(m_items[center]))
                            break;
                        ++center;
                    }
                    if(center == end) {
                        end = (start + end)/2; //No non-free items found in second half, so continue search in the other
                    }else{
                        if(data < m_items[center]) {
//                             Q_ASSERT(currentBound == -1 || currentBound > center);
                            currentBound = center;
                            end = (start + end)/2;
                        }else{
                            //Continue search in second half
                            start = center;
                        }
                    }
                }
            }

        uint countFreeItems() const {
            return countFreeItemsInternal(*m_centralFreeItem);
        }

        void verifyTreeConsistent() {
            verifyTreeConsistentInternal(*m_centralFreeItem, 0, m_itemCount);
        }
        
        private:
        void verifyTreeConsistentInternal(int position, int lowerBound, int upperBound) {
            if(position == -1)
                return;
            Q_ASSERT(lowerBound <= position && position < upperBound);
            verifyTreeConsistentInternal(ItemHandler::leftChild(m_items[position]), lowerBound, position);
            verifyTreeConsistentInternal(ItemHandler::rightChild(m_items[position]), position+1, upperBound);
        }
        
        uint countFreeItemsInternal(int item) const {
            if(item == -1)
                return 0;
            
            return 1 + countFreeItemsInternal(ItemHandler::leftChild(m_items[item])) + countFreeItemsInternal(ItemHandler::rightChild(m_items[item]));
        }

           const Data* m_items;
           uint m_itemCount;
           const int* m_centralFreeItem;
    };
    
    /**Use this to add items.
     * The added item must not be in the set yet!
     * General usage:
     * - Construct the object
     * - Check if newItemCount() equals the previous item-count. If not, construct
     *   a new list as large as newItemCount, and call object.transferData to transfer the data
     *   into the new list. The new size must match the returned newItemCount.
     * - Either call object.apply(), or let it be called automatically by the destructor.
     * */
    template<class Data, class ItemHandler >
    class EmbeddedTreeAddItem {

        public:
            
            EmbeddedTreeAddItem(Data* items, uint itemCount, int& centralFreeItem, const Data& add) : m_add(add), m_items(items), m_itemCount(itemCount), m_centralFreeItem(&centralFreeItem), m_applied(false) {
            }
            ~EmbeddedTreeAddItem() {
                if(!m_applied)
                    apply();
            }

            ///Check this to see whether a new item-count is needed. If this does not equal the given itemCount, then
            ///the data needs to be transferred into a new list using transferData
            uint newItemCount() const {
                if(*m_centralFreeItem == -1) {
                    //We have to increase the size. Always increase by 10%.
                    uint newItemCount = m_itemCount + (m_itemCount/10);
                    if(newItemCount == m_itemCount)
                        ++newItemCount;
                    
                    return newItemCount;
                }else{
                    return m_itemCount;
                }
            }
            
            ///Transfers the data into a new item-list. The size of the new item-list must equal newItemCount()
            void transferData(Data* newItems, uint newCount, int* newCentralFree = 0) {
                DEBUG_FREEITEM_COUNT
                //We only transfer into a new list when all the free items are used up
                Q_ASSERT(countFreeItems(*m_centralFreeItem) == 0);
                
                //Create a new list where the items from m_items are put into newItems, with the free items evenly
                //distributed, and a clean balanced free-tree.
                uint newFreeCount = newCount - m_itemCount;
                volatile uint freeItemRaster = newCount / newFreeCount;
                Q_ASSERT(freeItemRaster);
                uint offset = 0;
                for(uint a = 0; a < newCount; ++a) {
                    //Create new free items at the end of their raster range
                    if(a % freeItemRaster == (freeItemRaster-1)) {
                        //We need to insert a free item
                        ItemHandler::createFreeItem(newItems[a]);
                        ++offset;
                    }else{
                        newItems[a] = m_items[a-offset];
                    }
                }
                Q_ASSERT(m_itemCount+offset == newCount);
                
                m_items = newItems;
                m_itemCount = newCount;
                
                if(newCentralFree)
                    m_centralFreeItem  = newCentralFree;

                *m_centralFreeItem = buildFreeTree(newFreeCount, freeItemRaster, freeItemRaster-1);
               
                DEBUG_FREEITEM_COUNT
            }
            
           ///Puts the item into the list
           void apply() {
               Q_ASSERT(!m_applied);
               m_applied = true;
               Q_ASSERT(*m_centralFreeItem != -1);
               
               //Find the free item that is nearest to the target position in the item order
               int previousItem = -1;
               int currentItem = *m_centralFreeItem;
               int replaceCurrentWith = -1;
               
               //Now go down the chain, always into the items direction
               
               while(1) {
                   QPair<int, int> freeBounds = leftAndRightRealItems(currentItem);
                   const Data& current(m_items[currentItem]);
                   if(freeBounds.first != -1 && m_add < m_items[freeBounds.first]) {
                       //Follow left child
                       if(ItemHandler::leftChild(current) != -1) {
                           //Continue traversing
                           previousItem = currentItem;
                           currentItem = ItemHandler::leftChild(current);
                       }else{
                           replaceCurrentWith = ItemHandler::rightChild(current);
                           break;
                       }
                   }else if(freeBounds.second != -1 && m_items[freeBounds.second] < m_add) {
                       //Follow right child
                       if(ItemHandler::rightChild(current) != -1) {
                           //Continue traversing
                           previousItem = currentItem;
                           currentItem = ItemHandler::rightChild(current);
                       }else{
                           replaceCurrentWith = ItemHandler::leftChild(current);
                           break;
                       }
                   }else{
                       //We will use this item! So find a replacement for it in the tree, and update the structure
                       
                       int leftReplaceCandidate = -1, rightReplaceCandidate = -1;
                       if(ItemHandler::leftChild(current) != -1)
                           leftReplaceCandidate = rightMostChild(ItemHandler::leftChild(current));
                       if(ItemHandler::rightChild(current) != -1)
                           rightReplaceCandidate = leftMostChild(ItemHandler::rightChild(current));
                       
                       ///@todo it's probably better using lowerBound and upperBound like in the "remove" version
                       //Left and right bounds of all children of current
                       int leftChildBound = leftMostChild(currentItem), rightChildBound = rightMostChild(currentItem);
                       Q_ASSERT(leftChildBound != -1 && rightChildBound != -1);
                       int childCenter = (leftChildBound + rightChildBound)/2;
                       
                       if(leftReplaceCandidate == -1 && rightReplaceCandidate == -1) {
                          //We don't have a replace candidate, since there is no children
                          Q_ASSERT(ItemHandler::leftChild(current) == -1);
                          Q_ASSERT(ItemHandler::rightChild(current) == -1);
                       }else if(rightReplaceCandidate == -1 || abs(leftReplaceCandidate - childCenter) < abs(rightReplaceCandidate - childCenter)) {
                           //pick the left replacement, since it's more central
                           Q_ASSERT(leftReplaceCandidate != -1);
                           replaceCurrentWith = leftReplaceCandidate;
                           
                           Data& replaceWith(m_items[replaceCurrentWith]);
                           
                           if(replaceCurrentWith == ItemHandler::leftChild(current)) {
                               //The left child of replaceWith can just stay as it is, and we just need to add the right child
                               Q_ASSERT(ItemHandler::rightChild(replaceWith) == -1);
                           }else{
                            takeRightMostChild(ItemHandler::leftChild(current));
                            
                            //Since we'll be clearing the item, we have to put this childsomewhere else. 
                            // Either make it our new "left" child, or make it the new left children "rightmost" child.
                            int addRightMostLeftChild = ItemHandler::leftChild(replaceWith);
                            
                            ItemHandler::setLeftChild(replaceWith, -1);
                            
                            Q_ASSERT(ItemHandler::leftChild(replaceWith) == -1);
                            Q_ASSERT(ItemHandler::rightChild(replaceWith) == -1);
                            
                            if(ItemHandler::leftChild(current) != -1)
                            {
                                Q_ASSERT(rightMostChild(ItemHandler::leftChild(current)) != replaceCurrentWith);
                                Q_ASSERT(ItemHandler::leftChild(current) == -1 || ItemHandler::leftChild(current) < replaceCurrentWith);
                                ItemHandler::setLeftChild(replaceWith, ItemHandler::leftChild(current));
                                
                                if(addRightMostLeftChild != -1) {
                                    int rightMostLeft = rightMostChild(ItemHandler::leftChild(replaceWith));
                                    Q_ASSERT(rightMostLeft != -1);
//                                     Q_ASSERT(item(rightMostLeft).ItemHandler::rightChild() == -1);
                                    Q_ASSERT(rightMostLeft < addRightMostLeftChild);
                                    ItemHandler::setRightChild(m_items[rightMostLeft], addRightMostLeftChild);
                                }
                            }else{
                                Q_ASSERT(addRightMostLeftChild == -1 || addRightMostLeftChild < replaceCurrentWith);
                                ItemHandler::setLeftChild(replaceWith, addRightMostLeftChild);
                            }
                           }
                           
                           Q_ASSERT(ItemHandler::rightChild(current) == -1 || replaceCurrentWith < ItemHandler::rightChild(current));
                           ItemHandler::setRightChild(replaceWith, ItemHandler::rightChild(current));
                       }else{
                           //pick the right replacement, since it's more central
                           Q_ASSERT(rightReplaceCandidate != -1);
                           replaceCurrentWith = rightReplaceCandidate;
                           
                           Data& replaceWith(m_items[replaceCurrentWith]);
                           
                           if(replaceCurrentWith == ItemHandler::rightChild(current)) {
                               //The right child of replaceWith can just stay as it is, and we just need to add the left child
                               Q_ASSERT(ItemHandler::leftChild(replaceWith) == -1);
                           }else{
                            takeLeftMostChild(ItemHandler::rightChild(current));
                            
                            //Since we'll be clearing the item, we have to put this childsomewhere else. 
                            // Either make it our new "right" child, or make it the new right children "leftmost" child.
                            int addLeftMostRightChild = ItemHandler::rightChild(replaceWith);
                            
                            ItemHandler::setRightChild(replaceWith, -1);
                            
                            Q_ASSERT(ItemHandler::rightChild(replaceWith) == -1);
                            Q_ASSERT(ItemHandler::leftChild(replaceWith) == -1);
                            
                            if(ItemHandler::rightChild(current) != -1)
                            {
                                Q_ASSERT(leftMostChild(ItemHandler::rightChild(current)) != replaceCurrentWith);
                                Q_ASSERT(ItemHandler::rightChild(current) == -1 || replaceCurrentWith < ItemHandler::rightChild(current));
                                ItemHandler::setRightChild(replaceWith, ItemHandler::rightChild(current));
                                
                                if(addLeftMostRightChild != -1) {
                                    int leftMostRight = leftMostChild(ItemHandler::rightChild(replaceWith));
                                    Q_ASSERT(leftMostRight != -1);
                                    Q_ASSERT(ItemHandler::leftChild(m_items[leftMostRight]) == -1);
                                    Q_ASSERT(addLeftMostRightChild < leftMostRight);
                                    ItemHandler::setLeftChild(m_items[leftMostRight], addLeftMostRightChild);
                                }
                            }else{
                                Q_ASSERT(addLeftMostRightChild == -1 || replaceCurrentWith < addLeftMostRightChild);
                                ItemHandler::setRightChild(replaceWith, addLeftMostRightChild);
                            }
                           }
                           
                           Q_ASSERT(ItemHandler::leftChild(current) == -1 || ItemHandler::leftChild(current) < replaceCurrentWith);
                           ItemHandler::setLeftChild(replaceWith, ItemHandler::leftChild(current));
                       }
                       break;
                   }
               }
               
               //We can insert now
               //currentItem and previousItem are the two items that best enclose the target item
                
               //First, take currentItem out of the chain, by replacing it with current.rightChild in the parent
               if(previousItem != -1) {
                   Data& previous(m_items[previousItem]);
                    if(ItemHandler::leftChild(previous) == currentItem) {
                        Q_ASSERT(replaceCurrentWith == -1 || replaceCurrentWith < previousItem);
                        ItemHandler::setLeftChild(previous, replaceCurrentWith);
                    } else if(ItemHandler::rightChild(previous) == currentItem) {
                        Q_ASSERT(replaceCurrentWith == -1 || previousItem < replaceCurrentWith);
                        ItemHandler::setRightChild(previous, replaceCurrentWith);
                    } else {
                        Q_ASSERT(0);
                    }
               } else {
                   *m_centralFreeItem = replaceCurrentWith;
               }
               
               ItemHandler::copyTo(m_add, m_items[currentItem]);
               updateSorting(currentItem);
               DEBUG_FREEITEM_COUNT
           }
        
        private:
           void verifyTreeConsistent(int position, int lowerBound, int upperBound) {
               if(position == -1)
                   return;
               Q_ASSERT(lowerBound <= position && position < upperBound);
               verifyTreeConsistent(ItemHandler::leftChild(m_items[position]), lowerBound, position);
               verifyTreeConsistent(ItemHandler::rightChild(m_items[position]), position+1, upperBound);
           }
            
            void debugFreeItemCount() {
                uint count = 0;
                for(uint a = 0; a < m_itemCount; ++a) {
                    if(isFree(m_items[a]))
                        ++count;
                }
                uint counted = countFreeItems(*m_centralFreeItem);
                Q_ASSERT(count == counted);
            }
            
            QPair<int, int> leftAndRightRealItems(int pos) {
                Q_ASSERT(ItemHandler::isFree(m_items[pos]));
                int left = -1, right = -1;
                for(int a = pos-1; a >= 0; --a) {
                    if(!ItemHandler::isFree(m_items[a])) {
                        left = a;
                        break;
                    }
                }
                for(uint a = pos+1; a < m_itemCount; ++a) {
                    if(!ItemHandler::isFree(m_items[a])) {
                        right = a;
                        break;
                    }
                }
                return qMakePair(left, right);
            }
            
            int buildFreeTree(int count, uint raster, int start) {
                Q_ASSERT((start % raster) == (raster-1));
                Q_ASSERT(count != 0);
                Q_ASSERT(count <= (int)m_itemCount);
                if(count == 1) {
                    ItemHandler::createFreeItem(m_items[start]);
                    return start;
                }else{
                    int central = start + (count / 2) * raster;
                    int leftCount = count / 2;
                    int midCount = 1;
                    int rightCount = count - leftCount - midCount;
                    Q_ASSERT(leftCount + midCount <= count);
                    ItemHandler::createFreeItem(m_items[central]);
                    Q_ASSERT(ItemHandler::isFree(m_items[central]));
                    
                    int leftFreeTree = buildFreeTree(leftCount, raster, start);
                    Q_ASSERT(leftFreeTree == -1 || leftFreeTree < central);
                    ItemHandler::setLeftChild(m_items[central],  leftFreeTree );
                    
                    if(rightCount > 0) {
                        int rightFreeTree = buildFreeTree(rightCount, raster, central+raster);
                        Q_ASSERT(rightFreeTree == -1 || central < rightFreeTree);
                        ItemHandler::setRightChild(m_items[central], rightFreeTree );
                    }
                    
                    return central;
                }
            }
            
            uint countFreeItems(int item) const {
                if(item == -1)
                    return 0;
                const Data& current(m_items[item]);
                
                return 1 + countFreeItems(ItemHandler::leftChild(current)) + countFreeItems(ItemHandler::rightChild(current));
            }
            
           int leftMostChild(int pos) const {
               while(1) {
                   if(ItemHandler::leftChild(m_items[pos]) != -1)
                       pos = ItemHandler::leftChild(m_items[pos]);
                   else
                       return pos;
               }
           }
           
           int takeLeftMostChild(int pos) const {
               int parent = -1;
               while(1) {
                   if(ItemHandler::leftChild(m_items[pos]) != -1) {
                       parent = pos;
                       pos = ItemHandler::leftChild(m_items[pos]);
                   } else {
                       ItemHandler::setLeftChild(m_items[parent], -1);
                       return pos;
                   }
               }
           }

           int rightMostChild(int pos) const {
               while(1) {
                   if(ItemHandler::rightChild(m_items[pos]) != -1)
                       pos = ItemHandler::rightChild(m_items[pos]);
                   else
                       return pos;
               }
           }
           
           int takeRightMostChild(int pos) const {
               int parent = -1;
               while(1) {
                   if(ItemHandler::rightChild(m_items[pos]) != -1) {
                       parent = pos;
                       pos = ItemHandler::rightChild(m_items[pos]);
                   } else {
                       ItemHandler::setRightChild(m_items[parent], -1);
                       return pos;
                   }
               }
           }

           //Updates the sorting for this item locally, using bubble-sort
           void updateSorting(int pos) {
               while(1) {
                int prev = pos-1;
                int next = pos+1;
                if(prev >= 0 && !ItemHandler::isFree(m_items[prev]) && m_items[pos] < m_items[prev]) {
                    Data backup(m_items[prev]);
                    m_items[prev] = m_items[pos];
                    m_items[pos] = backup;
                    pos = prev;
                }else if(next < (int)m_itemCount && !ItemHandler::isFree(m_items[next]) && m_items[next] < m_items[pos]) {
                    Data backup(m_items[next]);
                    m_items[next] = m_items[pos];
                    m_items[pos] = backup;
                    pos = next;
                }else{
                    break;
                }
               }
           }
           
           const Data& m_add;
           Data* m_items;
           uint m_itemCount;
           int* m_centralFreeItem;
           bool m_applied;
    };

    /**Use this to add items.
     * The removed item must be in the set!
     * General usage:
     * - Construct the object
     * - Check if newItemCount() equals the previous item-count. If not, construct
     *   a new list as large as newItemCount, and call object.transferData to transfer the data
     *   into the new list. The new size must match the returned newItemCount.
     * However this may also be ignored if the memory-saving is not wanted in that moment.
     * */
    template<class Data, class ItemHandler >
    class EmbeddedTreeRemoveItem {

        public:
            
            EmbeddedTreeRemoveItem(Data* items, uint itemCount, int& centralFreeItem, const Data& remove) : m_remove(remove), m_items(items), m_itemCount(itemCount), m_centralFreeItem(&centralFreeItem) {
                 apply();
            }

            ~EmbeddedTreeRemoveItem() {
            }

            ///Check this to see whether a new item-count is needed. If this does not equal the given itemCount, then
            ///the data needs to be transferred into a new list using transferData
            uint newItemCount() const {
                uint freeCount = countFreeItems(*m_centralFreeItem);
                if(freeCount > ((m_itemCount / 7)+1) || freeCount == m_itemCount)
                    return m_itemCount - freeCount;
                else
                    return m_itemCount;
            }
            
            ///Transfers the data into a new item-list. The size of the new item-list must equal newItemCount()
            void transferData(Data* newItems, uint newCount, int* newCentralFree = 0) {
                DEBUG_FREEITEM_COUNT
                //We only transfer into a new list when all the free items are used up
                
                //Create a list where only the non-free items exist
                uint offset = 0;
                for(uint a = 0; a < m_itemCount; ++a) {
                    if(!ItemHandler::isFree(m_items[a])) {
                        newItems[offset] = m_items[a];
                        ++offset;
                    }
                }
                Q_ASSERT(offset == newCount);

                if(newCentralFree)
                    m_centralFreeItem = newCentralFree;
                *m_centralFreeItem = -1;
                m_items = newItems;
                m_itemCount = newCount;
                DEBUG_FREEITEM_COUNT
            }
            
        private:
           void verifyTreeConsistent(int position, int lowerBound, int upperBound) {
                if(position == -1)
                    return;
                Q_ASSERT(lowerBound <= position && position < upperBound);
                verifyTreeConsistent(ItemHandler::leftChild(m_items[position]), lowerBound, position);
                verifyTreeConsistent(ItemHandler::rightChild(m_items[position]), position+1, upperBound);
           }
            
            uint countFreeItems(int item) const {
                if(item == -1)
                    return 0;
                const Data& current(m_items[item]);
                
                return 1 + countFreeItems(ItemHandler::leftChild(current)) + countFreeItems(ItemHandler::rightChild(current));
            }
            
            int findItem(const Data& data, uint start, uint end) {
                EmbeddedTreeAlgorithms<Data, ItemHandler> alg(m_items, m_itemCount, *m_centralFreeItem);
                return alg.indexOf(data, start, end);
            }
            
           void apply() {
               DEBUG_FREEITEM_COUNT
               
               int removeIndex = findItem(m_remove, 0, m_itemCount);
               Q_ASSERT(removeIndex != -1);
               Q_ASSERT(!ItemHandler::isFree(m_items[removeIndex]));
               
               //Find the free item that is nearest to the target position in the item order
               int previousItem = -1;
               int currentItem = *m_centralFreeItem;
               
               int lowerBound = 0; //The minimum position the searched item can have
               int upperBound = m_itemCount; //The lowest known position the searched item can _not_ have
               
               if(*m_centralFreeItem == -1) {
                   *m_centralFreeItem = removeIndex;
                   Q_ASSERT(*m_centralFreeItem != -1);
                   ItemHandler::createFreeItem(m_items[*m_centralFreeItem]);
                   return;
               }
               
               //Now go down the chain, always into the items direction
               ///@todo make the structure better: Don't just put left/right child, but also swap when neede
               ///      to balance the tree
               while(1) {
                   Q_ASSERT(removeIndex != currentItem);
                   Data& current(m_items[currentItem]);
                   if(removeIndex < currentItem) {
                       upperBound = currentItem;
                       //Follow left child
                       if(ItemHandler::leftChild(current) != -1) {
                           //Continue traversing
                           previousItem = currentItem;
                           currentItem = ItemHandler::leftChild(current);
                           Q_ASSERT(currentItem >= lowerBound && currentItem < upperBound);
                       }else{
                           //The to-be deleted item is before current, and can be added as left child to current 
                           int item = findItem(m_remove, lowerBound, upperBound);
                           Q_ASSERT(item == removeIndex);
                           ItemHandler::createFreeItem(m_items[item]);
                           Q_ASSERT(item == -1 || item < currentItem);
                           ItemHandler::setLeftChild(current, item);
                           Q_ASSERT(item >= lowerBound && item < upperBound);
                           break;
                       }
                   }else{
                       lowerBound = currentItem+1;
                       //Follow right child
                       if(ItemHandler::rightChild(current) != -1) {
                           //Continue traversing
                           previousItem = currentItem;
                           currentItem = ItemHandler::rightChild(current);
                           Q_ASSERT(currentItem >= lowerBound && currentItem < upperBound);
                       }else{
                           //The to-be deleted item is behind current, and can be added as right child to current 
                           int item = findItem(m_remove, lowerBound, upperBound);
                           Q_ASSERT(item == removeIndex);
                           ItemHandler::createFreeItem(m_items[item]);
                           Q_ASSERT(item == -1 || currentItem < item);
                           ItemHandler::setRightChild(current, item);
                           Q_ASSERT(item >= lowerBound && item < upperBound);
                           break;
                       }
                   }
               }
               
               DEBUG_FREEITEM_COUNT
           }
            
           void debugFreeItemCount() {
               uint count = 0;
               for(uint a = 0; a < m_itemCount; ++a) {
                   if(ItemHandler::isFree(m_items[a]))
                       ++count;
               }
               uint counted = countFreeItems(*m_centralFreeItem);
               Q_ASSERT(count == counted);
           }
           
           //Updates the sorting for this item, using bubble-sort
           void updateSorting(int pos) {
               Q_ASSERT(0);
           }
           
           const Data& m_remove;
           Data* m_items;
           uint m_itemCount;
           int* m_centralFreeItem;
    };
}

#endif
