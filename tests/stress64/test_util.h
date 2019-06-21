/*
Copyright(c) 2019 Anatoliy Kuznetsov(anatoliy_kuznetsov at yahoo.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

For more information please visit:  http://bitmagic.io
*/


/// Load bit-vector using ref syntax
///
template<typename BV, typename VT>
void load_BV_set_ref(BV& bv, const VT& vect, bool print_stat = true)
{
    for (auto it = vect.begin(); it != vect.end(); ++it)
    {
        auto v = *it;
        bv[v] = true;
    }
    assert(bv.count() == vect.size());
    if (print_stat)
        print_bvector_stat(bv);
}

/// Load bit-vector using ref syntax
///
template<typename BV, typename VT>
void clear_BV_set_ref(BV& bv, const VT& vect, bool print_stat = true)
{
    for (auto it = vect.begin(); it != vect.end(); ++it)
    {
        auto v = *it;
        bv[v] = false;
    }
    if (print_stat)
        print_bvector_stat(bv);
}



/// CMP bit-vector using ref syntax
///
template<typename BV, typename VT>
void compare_BV_set_ref(const BV& bv, const VT& vect, bool compare_count=true)
{
    for (auto it = vect.begin(); it != vect.end(); ++it)
    {
        auto v = *it;
        bool b = bv[v];
        if (!b)
        {
            cerr << "Error! Vector(ref) comparison failed. v=" << v
                 << endl;
            assert(0);
            exit(1);
        }
    }
    if (compare_count)
    {
        auto count = bv.count();
        if (count != vect.size())
        {
            cerr << "Error! Vector(ref) size cmp failed. vect.size()=" << vect.size()
                << " bv.count()=" << count << endl;
            assert(0);
            exit(1);
        }
    }
}

/// CMP bit-vector using enumerator
///
template<typename BV, typename VT>
void compare_BV(const BV& bv, const VT& vect, bool compare_count = true)
{
    typename BV::enumerator en = bv.first();
    auto prev_id = 0ULL;
    for (auto it = vect.begin(); it != vect.end(); ++it, ++en)
    {
        auto v0 = *it;
        if (!en.valid())
        {
            cerr << "Error! Vector(en) comparison failed. enumerator invalid at value=" << v0
                 << endl;
            assert(0); exit(1);
        }
        auto v1 = *en;
        if (v1 != v0)
        {
            cerr << "Error! Vector(en) comparison failed. v0=" << v0
                 << " v1=" << v1
                 << endl;
            assert(0); exit(1);
        }
        if ((v1 != prev_id) && compare_count)
        {
            auto r = bv.count_range(prev_id, v1);
            if (r != 2ULL)
            {
                cerr << "Error! Vector(en) comparison failed. count_range() = " << r
                     << " [" << prev_id << ", " << v1 << "]"
                     << endl;
                assert(0); exit(1);
            }
        }
        prev_id = v1;
    }
    if (compare_count)
    {
        auto count = bv.count();
        if (count != vect.size())
        {
            cerr << "Error! Vector(en) size cmp failed. vect.size()=" << vect.size()
                << " bv.count()=" << count << endl;
            assert(0);
            exit(1);
        }
    }
}


template<class SV, class Vect>
bool CompareSparseVector(const SV& sv, const Vect& vect, bool interval_filled = false)
{
    if (vect.size() != sv.size())
    {
        cerr << "Sparse vector size test failed!" << vect.size() << "!=" << sv.size() << endl;
        return false;
    }
    
    if (sv.is_nullable())
    {
        const typename SV::bvector_type* bv_null = sv.get_null_bvector();
        assert(bv_null);
        auto non_null_cnt = bv_null->count();
        if (vect.size() != non_null_cnt)
        {
            if (!interval_filled)
            {
                cerr << "NULL vector count failed." << non_null_cnt << " size=" << vect.size() << endl;
                assert(0); exit(1);
            }
        }
    }
    
    {
        typename SV::const_iterator it = sv.begin();
        typename SV::const_iterator it_end = sv.end();

        for (unsigned i = 0; i < vect.size(); ++i)
        {
            typename Vect::value_type v1 = vect[i];
            typename SV::value_type v2 = sv[i];
            typename SV::value_type v3 = *it;

            if (v1 != v2)
            {
                cerr << "SV discrepancy:" << "sv[" << i << "]=" << v2
                     <<  " vect[" << i << "]=" << v1
                     << endl;
                return false;
            }
            if (v1 != v3)
            {
                cerr << "SV discrepancy:" << "sv[" << i << "]=" << v2
                     <<  " *it" << v3
                     << endl;
                return false;
            }
            assert(it < it_end);
            ++it;
        } // for
        if (it != it_end)
        {
            cerr << "sv const_iterator discrepancy!" << endl;
            return false;
        }
    }
    
    // extraction comparison
    {
        std::vector<typename SV::value_type> v1(sv.size());
        std::vector<typename SV::value_type> v1r(sv.size());
        sv.extract(&v1[0], sv.size(), 0);
        sv.extract_range(&v1r[0], sv.size(), 0);
        for (unsigned i = 0; i < sv.size(); ++i)
        {
            if (v1r[i] != v1[i] || v1[i] != vect[i])
            {
                cerr << "TestEqualSparseVectors Extract 1 failed at:" << i
                     << " v1[i]=" << v1[i] << " v1r[i]=" << v1r[i]
                     << endl;
                assert(0);exit(1);
            }
        } // for
    }
    
    // serialization comparison
    BM_DECLARE_TEMP_BLOCK(tb)
    bm::sparse_vector_serial_layout<SV> sv_lay;
    bm::sparse_vector_serialize<SV>(sv, sv_lay, tb);
    SV sv2;
    const unsigned char* buf = sv_lay.buf();
    int res = bm::sparse_vector_deserialize(sv2, buf, tb);
    if (res != 0)
    {
        cerr << "De-Serialization error" << endl;
        assert(0);exit(1);
    }
    if (sv.is_nullable() != sv2.is_nullable())
    {
        cerr << "Serialization comparison of two svectors failed (NULL vector)" << endl;
        assert(0);exit(1);
    }
    const typename SV::bvector_type* bv_null = sv.get_null_bvector();
    const typename SV::bvector_type* bv_null2 = sv.get_null_bvector();
    
    if (bv_null != bv_null2 && (bv_null == 0 || bv_null2 == 0))
    {
        cerr << "Serialization comparison (NUUL vector missing)!" << endl;
        assert(0);exit(1);
    }
    if (bv_null)
    {
        if (bv_null->compare(*bv_null2) != 0)
        {
            cerr << "Serialization comparison of two svectors (NULL vectors unmatch)!" << endl;
            assert(0);exit(1);
        }
    }
    if (!sv.equal(sv2) )
    {
        cerr << "Serialization comparison of two svectors failed" << endl;
        assert(0); exit(1);
    }
    return true;
}


template<typename SV, typename VT>
void load_SV_set_ref(SV* sv, const VT& vect)
{
    for (auto it = vect.begin(); it != vect.end(); ++it)
    {
        auto v = *it;
        sv->set(v, v);
    }
}


template<typename SV, typename VT>
void compare_SV_set_ref(const SV& sv, const VT& vect)
{
    for (size_t i = 0; i != vect.size(); ++i)
    {
        auto v = vect[i];
        auto vv = sv[v];
        if (v != vv)
        {
            std::cerr << "SV compare failed at:" << i
                      << " v=" << v << " sv[]=" << vv << std::endl;
            vv = sv[v];
            assert(v == vv);
            exit(1);
        }
    }
}


template<typename SV, typename VT>
void bulk_load_SV_set_ref(SV* sv, const VT& vect)
{
    assert(vect.size());
    typename SV::back_insert_iterator bi(sv->get_back_inserter());
    auto v_prev = vect[0];
    if (v_prev)
        bi.add_null(v_prev);
    *bi = v_prev;
    for (auto it = vect.begin(); it != vect.end(); ++it)
    {
        auto v = *it;
        if (v == v_prev)
            continue;
        assert(v > v_prev);
        typename SV::size_type diff = v - v_prev;
        if (diff > 1)
        {
            bi.add_null(diff-1);
        }
        *bi = v;
        v_prev = v;
    }
    bi.flush();
}

template<typename CSV, typename SV>
void DetailedCompareSparseVectors(const CSV& csv,
    const SV&            sv)
{
    SV   sv_s(bm::use_null);  // sparse vector decompressed

    // de-compression test
    csv.load_to(sv_s);
    /*
    if (!sv.equal(sv_s))
    {
        cerr << "compressed vector load_to (decompression) failed!" << endl;
        exit(1);
    }
    */


    size_t csv_size = csv.size();
    size_t sv_size = sv.size();
    size_t sv_s_size = sv_s.size();

    const typename SV::bvector_type* bv_null_sv = sv.get_null_bvector();
    const typename SV::bvector_type* bv_null_sv_s = sv_s.get_null_bvector();
    const typename SV::bvector_type* bv_null_csv = csv.get_null_bvector();

    if (csv_size != sv_size || sv_s_size != sv_size)
    {
        assert(bv_null_sv != bv_null_csv);

        auto cnt_sv = bv_null_sv->count();
        auto cnt_sv_s = bv_null_sv_s->count();
        auto cnt_csv = bv_null_csv->count();

        if (cnt_sv != cnt_csv)
        {
            cerr << "Sparse compressed vector comparison failed (size check):"
                << "csv.size()=" << csv_size
                << "sv.size()=" << sv_size
                << "cnt sv = " << cnt_sv
                << "cnt csv = " << cnt_csv
                << endl;
            assert(0); exit(1);
        }
        if (cnt_sv_s != cnt_csv)
        {
            cerr << "Restored Sparse vector comparison failed (size check):"
                << "csv.size()=" << csv_size
                << "sv_s.size()=" << sv_s_size
                << "cnt sv = " << cnt_sv
                << "cnt csv = " << cnt_csv
                << endl;
            assert(0); exit(1);
        }
    }

    for (typename SV::size_type i = 0; i < sv_size; ++i)
    {
        bool is_null_sv = sv.is_null(i);
        bool is_null_sv_s = sv_s.is_null(i);
        bool is_null_csv = csv.is_null(i);
        if (is_null_sv != is_null_csv || is_null_sv != is_null_sv_s)
        {
            cerr << "Detailed csv check failed (null mismatch) at i=" << i
                << " sv=" << is_null_sv
                << " sv_s=" << is_null_sv_s
                << " csv=" << is_null_csv
                << endl;
            int cmp = bv_null_sv->compare(*bv_null_csv);
            if (cmp != 0)
            {
                cerr << "1. cmp=" << cmp << endl;
                exit(1);
            }
            cmp = bv_null_sv->compare(*bv_null_sv_s);
            if (cmp != 0)
            {
                cerr << "2. cmp=" << cmp << endl;
                exit(1);
            }

            assert(0); exit(1);
        }


        if (!is_null_sv)
        {
            auto v1 = sv[i];
            auto v1_s = sv_s[i];
            auto v2 = csv[i];

            if (v1 != v2 || v1_s != v1)
            {
                cerr << "Detailed csv check failed (value mismatch) at i=" << i
                    << " v1=" << v1
                    << " v1_s=" << v1_s
                    << " v2=" << v2
                    << endl;
                assert(0);  exit(1);
            }
        }
    }

    {
        BM_DECLARE_TEMP_BLOCK(tb)
        bm::sparse_vector_serial_layout<CSV> sv_lay;
        bm::sparse_vector_serialize<CSV>(csv, sv_lay, tb);

        CSV csv1;
        const unsigned char* buf = sv_lay.buf();
        bm::sparse_vector_deserialize(csv1, buf, tb);

        if (!csv.equal(csv1))
        {
            cerr << "Conpressed sparse vector serialization comparison failed!" << endl;
            assert(0); exit(1);
        }
    }

}



template<typename CSV>
void CheckCompressedDecode(const CSV& csv,
    typename CSV::size_type from, typename CSV::size_type size)
{
    std::vector<typename CSV::value_type> vect;
    vect.resize(size);

    typename CSV::size_type sz = csv.decode(&vect[0], from, size);
    typename CSV::size_type ex_idx = 0;
    for (typename CSV::size_type i = from; i < from + sz; ++i)
    {
        auto v = csv.get(i);
        auto vx = vect[ex_idx];
        if (v != vx)
        {
            cerr << "compressed vector decode mismatch from="
                << from << " idx=" << i
                << " v=" << v << " vx=" << vx
                << endl;
            assert(0);  exit(1);
        }
        ++ex_idx;
    }
}

template<typename CSV>
void DetailedCheckCompressedDecode(const CSV& csv)
{
    auto size = csv.size();
    cout << endl;

    {
        typename CSV::size_type size1 = 100;
        for (typename CSV::size_type i = 0; i < size1; )
        {
            CheckCompressedDecode(csv, i, size);
            if (i % 128 == 0)
                cout << "\r" << i << "/" << size1 << flush;
            i++;
        }
    }
    cout << endl;

    {
        typename CSV::size_type size1 = 100000;
        for (typename CSV::size_type i = 0; i < size1; )
        {
            CheckCompressedDecode(csv, i, size1);
            cout << "\r" << i << "/" << size1 << flush;
            i += rand() % 3;
            size1 -= rand() % 5;
        }
    }
    cout << endl;

    {
        typename CSV::size_type size1 = size;
        for (typename CSV::size_type i = size - size / 2; i < size1; )
        {
            CheckCompressedDecode(csv, i, size1);
            cout << "\r" << i << "/" << size1 << flush;
            i += (1 + i);
        }
    }
    cout << endl;

    for (typename CSV::size_type i = size - size / 2; i < size; )
    {
        CheckCompressedDecode(csv, i, size);
        cout << "\r" << i << "/" << size << flush;
        i += rand() % 25000;
    }
    cout << endl;

    for (typename CSV::size_type i = size - size / 2; i < size; )
    {
        if (size <= i)
            break;
        CheckCompressedDecode(csv, i, size);
        cout << "\r" << i << "/" << size << flush;
        i += rand() % 25000;
        size -= rand() % 25000;;
    }
    cout << endl;

}
