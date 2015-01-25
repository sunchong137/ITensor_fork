//
// Distributed under the ITensor Library License, Version 1.2
//    (See accompanying LICENSE file.)
//
#ifndef __ITENSOR_IQTDATA_H
#define __ITENSOR_IQTDATA_H

#include "itdata.h"
#include "../iqindex.h"
#include "../indexset.h"

namespace itensor {

template<typename T>
class IQTData : public ITDispatch<IQTData<T>>
    {
    public:

    struct BlockOffset
        {
        long block = 0;
        long offset = 0;
        BlockOffset(long b, long o) : block(b), offset(o) { }
        };

    //////////////
    std::vector<BlockOffset> offsets;
    std::vector<T> data;
    //////////////

    IQTData(const IQIndexSet& is, 
            const QN& Q);

    template<typename Indexable>
    const T*
    getBlock(const IQIndexSet& is,
             const Indexable& block_ind) const;

    template<typename Indexable>
    T*
    getBlock(const IQIndexSet& is,
             const Indexable& block_ind)
        {
        //ugly but safe, efficient, and avoids code duplication (Meyers, Effective C++)
        return const_cast<T*>(static_cast<const IQTData&>(*this).getBlock(is,block_ind));
        }

    template<typename Indexable>
    const T*
    getElt(const IQIndexSet& is,
           const Indexable& ind) const;

    template<typename Indexable>
    T*
    getElt(const IQIndexSet& is,
           const Indexable& ind)
        {
        return const_cast<T*>(static_cast<const IQTData&>(*this).getElt(is,ind));
        }

    virtual
    ~IQTData() { }

    };

template<typename T>
IQTData<T>::
IQTData(const IQIndexSet& is, 
        const QN& Q)
    {
    if(is.r()==0)
        {
        data.assign(1,0);
        offsets.emplace_back(0,0);
        return;
        }
    detail::GCounter C(0,is.r()-1,0);
    for(int j = 0; j < is.r(); ++j) 
        C.setInd(j,0,is[j].nindex()-1);

    long totalsize = 0;
    for(; C.notDone(); ++C)
        {
        QN blockqn;
        for(int j = 0; j < is.r(); ++j)
            {
            const auto& J = is[j];
            auto i = C.i.fast(j);
            blockqn += J.qn(1+i)*J.dir();
            }
        if(blockqn == Q)
            {
            //PRI(C.i);
            //println("blockqn = ",blockqn);
            long indstr = 1, //accumulate Index strides
                 ind = 0,
                 totm = 1;   //accumulate area of Indices
            for(int j = 0; j < is.r(); ++j)
                {
                const auto& J = is[j];
                auto i_j = C.i[j];
                ind += i_j*indstr;
                indstr *= J.nindex();
                totm *= J[i_j].m();
                }
            offsets.emplace_back(ind,totalsize);
            totalsize += totm;
            }
        }
    //print("offsets = {");
    //for(const auto& i : offsets)
    //    printf("(%d,%d),",i.block,i.offset);
    //println("}");
    //printfln("totalsize = %d",totalsize);
    data.assign(totalsize,0);
    }

template<typename T>
template<typename Indexable>
const T* IQTData<T>::
getBlock(const IQIndexSet& is,
         const Indexable& block_ind) const
    {
    auto r = long(block_ind.size());
    if(r == 0) return data.data();
#ifdef DEBUG
    if(is.r() != r) Error("Mismatched size of IQIndexSet and block_ind in get_block");
#endif
    long ii = 0;
    for(auto i = r-1; i > 0; --i)
        {
        ii += block_ind[i];
        ii *= is[i-1].nindex();
        }
    ii += block_ind[0];
    //Do a linear search to see if there
    //is a block with block index ii
    for(const auto& io : offsets)
        if(io.block == ii)
            {
            return data.data()+io.offset;
            }
    return nullptr;
    }

template<typename T>
template<typename Indexable>
const T* IQTData<T>::
getElt(const IQIndexSet& is,
       const Indexable& ind) const
    {
    auto r = long(ind.size());
    if(r == 0) return data.data();
#ifdef DEBUG
    if(is.r() != r) Error("Mismatched size of IQIndexSet and elt_ind in get_block");
#endif
    long boff = 0, //block offset
         bstr = 1, //block stride so far
         eoff = 0, //element offset within block
         estr = 1; //element stride
    for(auto i = 0; i < r; ++i)
        {
        //println("i=",i);
        const auto& I = is[i];
        long block_ind = 0,
             elt_ind = ind[i];
        //printfln("  elt_ind = %d, I[%d].m()=%d",elt_ind,block_ind,I[block_ind].m());
        while(elt_ind >= I[block_ind].m()) //elt_ind 0-indexed
            {
            elt_ind -= I[block_ind].m();
            ++block_ind;
            }
        //printfln("  block_ind=%d",block_ind);
        boff += block_ind*bstr;
        bstr *= I.nindex();
        eoff += elt_ind*estr;
        estr *= I[block_ind].m();
        }
    //Do a linear search to see if there
    //is a block with block offset bo
    for(const auto& io : offsets)
        if(io.block == boff)
            {
#ifdef DEBUG
            if(io.offset+eoff >= data.size()) Error("get_elt out of range");
#endif
            //printfln("io.offset = %d, eoff = %d, total = %d",io.offset,eoff,io.offset+eoff);
            return data.data()+io.offset+eoff;
            }
    return nullptr;
    }

}; //namespace itensor

#endif
