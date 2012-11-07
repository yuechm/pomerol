//
// This file is a part of pomerol - a scientific ED code for obtaining 
// properties of a Hubbard model on a finite-size lattice 
//
// Copyright (C) 2010-2011 Andrey Antipov <Andrey.E.Antipov@gmail.com>
// Copyright (C) 2010-2011 Igor Krivenko <Igor.S.Krivenko@gmail.com>
//
// pomerol is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// pomerol is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with pomerol.  If not, see <http://www.gnu.org/licenses/>.


/** \file Operator.cpp
**  \brief Implementation of the Operator, Operator::Term classes
** 
**  \author    Andrey Antipov (Andrey.E.Antipov@gmail.com)
*/

#include "Operator.h"
#include <boost/tuple/tuple.hpp>
#include <boost/utility.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

namespace Pomerol{

//
//Operator::Term
//

Operator::Term::Term (const unsigned int N, const std::vector<bool>&  Sequence, const std::vector<ParticleIndex> & Indices, MelemType Value):
    N(N), OperatorSequence(Sequence), Indices(Indices), Value(Value)
{
    if (Sequence.size()!=N || Indices.size()!=N) throw(exWrongLabel());
    for (unsigned int i=0; i<N; ++i) {  
        int count_index=2*Sequence[i]-1; // This determines how many times current index Indices[i] is found. c^+ gives +1, c gives -1.
        //DEBUG(i << " " << Indices[i] << " " << count_index);
        for (unsigned int j=i+1; j<N; ++j)
            if (Indices[i]==Indices[j] ) { 
                count_index+=2*Sequence[j]-1; 
                //DEBUG(j << " " << Indices[j] << " " << count_index);
                if ( count_index > 1 || count_index < -1 ) { ERROR("This term vanishes. "); throw (exWrongOpSequence()); }; 
            } 
        };    
}

boost::shared_ptr<std::list<Operator::Term*> > Operator::Term::rearrange(const std::vector<bool> & DesiredSequence)
{
    if (DesiredSequence.size() != OperatorSequence.size() ) throw (exWrongOpSequence());
    boost::shared_ptr<std::list<Operator::Term*> > out ( new std::list<Operator::Term*> );
    if (OperatorSequence == DesiredSequence) { return out; } // Nothing is needed to do then.

    for (unsigned int i=0; i<N-1; ++i) { 
        if (OperatorSequence[i]!=DesiredSequence[i]) {
            unsigned int j=i+1;
            for (; j<N && ( OperatorSequence[j] == OperatorSequence[i] || OperatorSequence[j] == DesiredSequence[j]); ++j) {};  // finding element to change
            if (j==N) throw (exWrongOpSequence()); // exit if there is no way of doing the rearrangement.
            if (N==2) { elementary_swap(0,true); return out; }; // If there are only two operators - just swap them and go away.

            // Now check if any operations with anticommutation relation is required.
            bool needNewTerms=false;
            for (unsigned int k=i+1; k<j && !needNewTerms; k++) needNewTerms = (Indices[k]==Indices[i] || Indices[k]==Indices[j]);

            if ( !needNewTerms ) { // Operators anticommute then. Constant energy levels are omitted.
                    Value*=(-1.); 
                    OperatorSequence[i]=!OperatorSequence[i]; 
                    OperatorSequence[j]=!OperatorSequence[j]; 
                    std::swap(Indices[i], Indices[j] );
                    }
            else { // Operators do not anticommute - swap will construct additional term.
                for (unsigned int k=j-1; k>=i; k--) { // move an operator at position j to the left to the position i.
                    std::list<Operator::Term*> out_temp = *(elementary_swap(k)); 
                    out->resize(out->size()+out_temp.size());
                    std::copy_backward(out_temp.begin(), out_temp.end(), out->end()); 
                    if (k==0) break; // exit, since unsigned int loop is done.
                    };
                for (unsigned int k=i+1; k<j; k++) { // move the operator at position i+1 to the right to the position j.
                    std::list<Operator::Term*> out_temp = *(elementary_swap(k)); 
                    out->resize(out->size()+out_temp.size());
                    std::copy_backward(out_temp.begin(), out_temp.end(), out->end()); 
                    };
                }; // end of else
            }; // end of element check
        } // end of i loop
    // Now we have the right order of booleans - what about indices? Indices should be rearranged prior to this
    return out;
}

void Operator::Term::reorder(bool ascend)
{
    //DEBUG("reordering " << *this);
    if (!N) return;
    assert ( OperatorSequence[0]==1 && OperatorSequence[N-1]==0);
    for (unsigned int i=0; i<N/2; ++i)
        for (unsigned int j=i*(!ascend); j<N/2-i*ascend-1; ++j)
        {
            if (ascend) {
                if (Indices[j+1] < Indices[j]) elementary_swap(j,true);
                if (Indices[j+1+N/2] < Indices[j+N/2]) elementary_swap(j+N/2,true);
                }
            else {
                if (Indices[j+1] > Indices[j]) elementary_swap(j,true);
                if (Indices[j+1+N/2] > Indices[j+N/2]) elementary_swap(j+N/2,true); // indices are unsigned int, so this is stable
            }
        }
}

boost::shared_ptr<std::list<Operator::Term*> > Operator::Term::makeNormalOrder()
{
    //boost::shared_ptr<std::list<Operator::Term*> > out ( new std::list<Operator::Term*> );
    //return out;
    std::vector<bool> normalOrderedSequence;
    for (ParticleIndex i=0; i<N; ++i) {
        if ( !OperatorSequence[i] ) normalOrderedSequence.push_back(0);
        else normalOrderedSequence.insert(normalOrderedSequence.begin(),1);
        //else normalOrderedSequence.resize(normalOrderedSequence.size()+1,1); // for a bitset
        }
    boost::shared_ptr<std::list<Operator::Term*> > out = this->rearrange(normalOrderedSequence);
    //DEBUG("Main: " << *this);
    this->reorder();
    for (std::list<Operator::Term*>::iterator iter_it = out->begin(); iter_it!=out->end(); ++iter_it) {
            //DEBUG("Additional: " << **iter_it);
            boost::shared_ptr<std::list<Operator::Term*> > out2 = (*iter_it)->makeNormalOrder();
            out->splice(boost::prior(out->end()),*out2);
        }
    return out;
}

boost::shared_ptr<std::list<Operator::Term*> > Operator::Term::elementary_swap(unsigned int position, bool force_ignore_commutation)
{
    boost::shared_ptr<std::list<Operator::Term*> > out ( new std::list<Operator::Term*> );
    if ( Indices[position] != Indices[position+1] || force_ignore_commutation ) {
        Value*=(-1.); 
        bool tmp = OperatorSequence[position];
        OperatorSequence[position]=OperatorSequence[position+1];
        OperatorSequence[position+1]=tmp;
        std::swap(Indices[position], Indices[position+1] );
        }
    else {
        std::vector<bool> Seq2(N-2);
        std::vector<unsigned int> Ind2(N-2);
        for (unsigned int i=0; i<position; ++i) { Seq2[i] = OperatorSequence[i]; Ind2[i] = Indices[i]; };
        for (unsigned int i=position+2; i<N; ++i) { Seq2[i-2] = OperatorSequence[i]; Ind2[i-2] = Indices[i]; };
        out->push_back(new Term(N-2, Seq2, Ind2, Value));
        elementary_swap(position, true);
         };
    return out;
}

MelemType Operator::Term::getMatrixElement( const FockState & bra, const FockState &ket){
    MelemType result;
    FockState bra2;
    boost::tie(bra2, result) = this->actRight(ket);
    return (bra2 == bra)?result:0;
}

boost::tuple<FockState, MelemType> Operator::Term::actRight ( const FockState &ket )
{
    ParticleIndex prev_pos_ = 0; // Here we'll store the index of the last operator to speed up sign counting
    int sign=1;
    FockState bra = ket;
    for (int i=N-1; i>=0; i--) // Is the number of operator in OperatorSequence. Now we need to count them from back.
        {
            if (OperatorSequence[i] == bra[Indices[i]] ) return boost::make_tuple(ERROR_FOCK_STATE, 0); // This is Pauli principle.
            bra[Indices[i]] = OperatorSequence[i]; // This is c or c^+ acting
            if (Indices[i] > prev_pos_) 
                for (ParticleIndex j=prev_pos_; j<Indices[i]; ++j) { if (ket[j]) sign*=-1; } 
            else
                for (ParticleIndex j=prev_pos_; j>Indices[i]; j--) { if (ket[j]) sign*=-1; }
            
            //for (ParticleIndex j=0; j<Indices[i]; ++j) { if (ket[j]) sign*=-1; } 
        }
    return boost::make_tuple(bra, this->Value*MelemType(sign));
}

unsigned int Operator::Term::getN(){
    return N;
}

const char* Operator::Term::exWrongLabel::what() const throw(){
    return "Wrong labels";
};

const char* Operator::Term::exWrongOpSequence::what() const throw(){
    std::stringstream s;
    s << "The term has wrong operator sequence!";
    return s.str().c_str();
};

std::ostream& operator<< (std::ostream& output, const Operator::Term& out)
{
    output << out.Value << "*"; 
    for (unsigned int i=0; i<out.N; ++i) output << ((out.OperatorSequence[i])?"c^{+}":"c") << "_" << out.Indices[i];
    return output; 
}

bool Operator::Term::isExactlyEqual(const Operator::Term &rhs) const
{
    bool out=(N==rhs.N && Value==rhs.Value);
    if (!out) return false;
    for (unsigned int i=0; i<N; ++i) { out = (out && OperatorSequence[i] == rhs.OperatorSequence[i] && Indices[i] == rhs.Indices[i]); };
    return out;
}

bool Operator::Term::operator==(const Operator::Term &rhs) const
{
    bool out=(rhs.isExactlyEqual(*this));
    if (out) return true;
    Operator::Term this_copy(*this);
    Operator::Term rhs_copy(rhs);
    boost::shared_ptr<std::list<Operator::Term*> > list_lhs = this_copy.makeNormalOrder();
    boost::shared_ptr<std::list<Operator::Term*> > list_rhs = rhs_copy.makeNormalOrder();
    reduce(list_lhs);
    reduce(list_rhs);
//    DEBUG(this_copy << "," << rhs_copy);
//    DEBUG(list_lhs->size());
    if (!(this_copy.isExactlyEqual(rhs_copy)) || list_lhs->size() != list_rhs->size() ) return false;
    out = true;
    for ( std::pair<std::list<Operator::Term*>::iterator, std::list<Operator::Term*>::iterator> pair_it = std::make_pair(list_lhs->begin(), list_rhs->begin()); 
          pair_it.first!=list_lhs->end() && pair_it.second!=list_rhs->end(); ) { 
//        DEBUG(**pair_it.first);
//        DEBUG(**pair_it.second);
        out = (out && (**pair_it.first).isExactlyEqual(**pair_it.second));
        pair_it.first++; pair_it.second++;
        }
    return out;
}



boost::shared_ptr<std::list<Operator::Term*> > Operator::Term::getCommutator(const Operator::Term &rhs) const
{
    boost::shared_ptr<std::list<Operator::Term*> > out ( new std::list<Operator::Term*> );
    int Ntotal = N+rhs.N;
    
    std::vector<bool> Seq2(Ntotal);
    std::vector<unsigned int> Ind2(Ntotal);

    for (unsigned int i=0; i<N; ++i)      { Seq2[i] = OperatorSequence[i]; Ind2[i] = Indices[i]; };
    for (unsigned int i=N; i<Ntotal; ++i) { Seq2[i] = rhs.OperatorSequence[i-N]; Ind2[i] = rhs.Indices[i-N]; };
    try {
            out->push_back(new Term(Ntotal, Seq2, Ind2, Value));
        }
    catch (Operator::Term::exWrongOpSequence &e)
        {
            ERROR("Commutator [" << *this << "," << rhs << "] creates a vanishing term");
        }

    for (unsigned int i=0; i<rhs.N; ++i)      { Seq2[i] = rhs.OperatorSequence[i]; Ind2[i] = rhs.Indices[i]; };
    for (unsigned int i=rhs.N; i<Ntotal; ++i) { Seq2[i] = OperatorSequence[i-rhs.N]; Ind2[i] = Indices[i-rhs.N]; };
    try {
            out->push_back(new Term(Ntotal, Seq2, Ind2, -Value));
        }
    catch (Operator::Term::exWrongOpSequence &e)
        {
            ERROR("Commutator [" << *this << "," << rhs << "] creates a vanishing term");
        };

    return out;
}

bool Operator::Term::commutes(const Operator::Term &rhs) const
{
    boost::shared_ptr<std::list<Operator::Term*> > out_terms = this->getCommutator(rhs);
    if (out_terms->size() == 0) return true;
    if (out_terms->size() == 1) return false;
    assert (out_terms->size() == 2);
    Operator::Term* term1 = (*out_terms->begin());
    Operator::Term* term2 = (*out_terms->begin()++);
    return (*term1 == *term2);
}

void Operator::Term::reduce(boost::shared_ptr<std::list<Operator::Term*> > Terms)
{
    int it1_pos=0;
    for (std::list<Operator::Term*>::iterator it1 = Terms->begin(); it1!=Terms->end(); it1++) {
        for (std::list<Operator::Term*>::iterator it2 = boost::next(it1); it2!=Terms->end();) {
            if (it2!=Terms->end()) {
                if ((**it2).OperatorSequence == (**it1).OperatorSequence && (**it2).Indices == (**it1).Indices) {
                    (**it1).Value+=(**it2).Value;
                    it2 = Terms->erase(it2);
                    it1 = Terms->begin();
                    std::advance(it1,it1_pos);
                    }
                else it2++;
                }
            }
        it1_pos++;
    }
}

void Operator::Term::prune(boost::shared_ptr<std::list<Operator::Term*> > Terms, const RealType &Precision)
{
    //std::remove_if (Terms->begin(), Terms->end(), std::abs(boost::bind(boost::lambda::_1)->Value) < Precision);
    for (std::list<Operator::Term*>::iterator it1 = Terms->begin(); it1!=Terms->end(); it1++)
        if (std::abs((**it1).Value) < Precision) it1=Terms->erase(it1);
}

//
// Operator
//

Operator::Operator()
{
    Terms.reset( new std::list<Operator::Term*> );
}

Operator::Operator(boost::shared_ptr<std::list<Operator::Term*> > Terms) : Terms(Terms)
{
}


void Operator::printAllTerms() const
{
    for (std::list<Operator::Term*>::const_iterator it = Terms->begin(); it!=Terms->end(); it++)
        {
            INFO(**it);
        }
}

boost::shared_ptr<std::list<Operator::Term*> > Operator::getTerms() const
{
    return Terms;
}

MelemType Operator::getMatrixElement( const FockState & bra, const FockState &ket) const
{
    std::map<FockState, MelemType> output = this->actRight(ket);
    if (output.find(bra)==output.end()) 
        return 0;
    else { 
        return output[bra];
        }
}

std::map<FockState, MelemType> Operator::actRight(const FockState &ket) const
{
    std::map<FockState, MelemType> result1;
    for (std::list<Operator::Term*>::const_iterator it = Terms->begin(); it!=Terms->end(); it++)
        {
            FockState bra; 
            MelemType melem;
            boost::tie(bra,melem) = (*it)->actRight(ket);
            if (bra!=ERROR_FOCK_STATE && std::abs(melem)>std::numeric_limits<RealType>::epsilon()) 
                result1[bra]+=melem;
        }
    for (std::map<FockState, MelemType>::iterator it1 = result1.begin(); it1!=result1.end(); it1++) 
        if ( std::abs(it1->second)<std::numeric_limits<RealType>::epsilon() ) result1.erase(it1); 
    return result1;
}

Operator::~Operator()
{
    Terms.reset();
}

void Operator::makeNormalOrder()
{
    std::list<Operator::Term*> output; // Here we will store the normal ordered output terms.
    for (std::list<Operator::Term*>::const_iterator it = Terms->begin(); it!=Terms->end(); it++) {
        boost::shared_ptr<std::list<Operator::Term* > > out = (**it).makeNormalOrder();
        output.splice(boost::prior(output.end()),*out);
    }
    Terms->splice(boost::prior(Terms->end()),output);
    this->reduce();
    this->prune();
}

void Operator::reduce()
{
    Operator::Term::reduce(Terms);
}

void Operator::prune(const RealType &Precision)
{
    Operator::Term::prune(Terms,Precision);
}

std::ostream& operator<< (std::ostream& output, const Operator& out)
{
    for (std::list<Operator::Term*>::const_iterator it = out.Terms->begin(); it!=out.Terms->end(); it++) {
        output << **it << " ";
        };
    return output;
}

Operator Operator::getCommutator(const Operator &rhs) const
{
   // DEBUG("!!" << rhs.Terms->size());
    boost::shared_ptr<std::list<Operator::Term*> > output ( new std::list<Operator::Term*>);
    for (std::list<Operator::Term*>::const_iterator it = Terms->begin(); it!=Terms->end(); it++)
        for (std::list<Operator::Term*>::const_iterator it2 = rhs.Terms->begin(); it2!=rhs.Terms->end(); it2++) {
   //         DEBUG("Commuting" << **it << " and " << **it2);
            boost::shared_ptr<std::list<Operator::Term* > > out2 = (**it).getCommutator(**it2); 
    //        DEBUG(out2->size() << " terms generated.");
            output->splice(boost::prior(output->end()),*out2);
    //        DEBUG(output->size() << " in output.");
    }
    Operator out(output);
    return out;
}

bool Operator::commutes(const Operator &rhs) const
{
    bool out;
    Operator out_return(this->getCommutator(rhs));
    INFO("+++++++");
    out_return.printAllTerms();
    INFO("+++++++");
    out_return.makeNormalOrder();
    out_return.printAllTerms();
    INFO("+++++++");
    out_return.reduce();
    out_return.prune();
    out_return.printAllTerms();
//    DEBUG(out_return.Terms->size());
    return out;
}


} // end of namespace Pomerol
