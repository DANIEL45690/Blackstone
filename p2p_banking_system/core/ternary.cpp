#include "ternary.h"
#include <algorithm>
#include <sstream>

std::vector<int> TernaryCodingSystem::toBalancedTernary(int64_t n)
{
    std::vector<int> res;
    while (n != 0)
    {
        int r = n % 3;
        if (r == 2)
        {
            r = -1;
            n += 1;
        }
        else if (r == -2)
        {
            r = 1;
            n -= 1;
        }
        res.push_back(r);
        n /= 3;
    }
    if (res.empty())
        res.push_back(0);
    std::reverse(res.begin(), res.end());
    return res;
}

int64_t TernaryCodingSystem::fromBalancedTernary(const std::vector<int> &t)
{
    int64_t v = 0;
    for (int d : t)
        v = v * 3 + d;
    return v;
}

std::vector<int> TernaryCodingSystem::toTernary(uint64_t n)
{
    std::vector<int> res;
    if (n == 0)
        res.push_back(0);
    while (n > 0)
    {
        res.push_back(n % 3);
        n /= 3;
    }
    std::reverse(res.begin(), res.end());
    return res;
}

uint64_t TernaryCodingSystem::fromTernary(const std::vector<int> &t)
{
    uint64_t v = 0;
    for (int d : t)
        v = v * 3 + d;
    return v;
}

std::string TernaryCodingSystem::encodeBinary(const std::string &data)
{
    std::string out;
    out.reserve(data.size() * 7);
    for (unsigned char c : data)
    {
        auto t = toTernary(c);
        for (int x : t)
            out.push_back('0' + x);
        out.push_back('|');
    }
    return out;
}

std::string TernaryCodingSystem::decodeTernary(const std::string &enc)
{
    std::string out;
    std::vector<int> cur;
    cur.reserve(16);
    out.reserve(enc.size() / 4);
    for (char ch : enc)
    {
        if (ch == '|')
        {
            out.push_back(static_cast<char>(fromTernary(cur)));
            cur.clear();
        }
        else if (ch >= '0' && ch <= '2')
            cur.push_back(ch - '0');
    }
    return out;
}

std::string TernaryCodingSystem::encodeBalanced(const std::string &data)
{
    std::string out;
    for (unsigned char c : data)
    {
        auto t = toBalancedTernary(c);
        for (int x : t)
        {
            if (x == -1)
                out.push_back('T');
            else
                out.push_back('0' + x);
        }
        out.push_back('|');
    }
    return out;
}

std::string TernaryCodingSystem::decodeBalanced(const std::string &enc)
{
    std::string out;
    std::vector<int> cur;
    for (char ch : enc)
    {
        if (ch == '|')
        {
            out.push_back(static_cast<char>(fromBalancedTernary(cur)));
            cur.clear();
        }
        else if (ch == 'T')
            cur.push_back(-1);
        else if (ch >= '0' && ch <= '2')
            cur.push_back(ch - '0');
    }
    return out;
}

std::vector<std::vector<int>> TernaryCodingSystem::buildTernaryTree(const std::string &data, int depth)
{
    int totalNodes = (int)(pow(3, depth + 1) - 1) / 2;
    std::vector<std::vector<int>> tree(depth + 1);
    for (int i = 0; i <= depth; i++)
    {
        tree[i].resize((int)pow(3, i), 0);
    }
    for (size_t i = 0; i < data.size() && i < tree[depth].size(); i++)
    {
        tree[depth][i] = data[i] % 3;
    }
    for (int d = depth - 1; d >= 0; d--)
    {
        for (int i = 0; i < (int)tree[d].size(); i++)
        {
            int sum = 0;
            for (int j = 0; j < 3; j++)
            {
                if (i * 3 + j < (int)tree[d + 1].size())
                {
                    sum += tree[d + 1][i * 3 + j];
                }
            }
            tree[d][i] = sum % 3;
        }
    }
    return tree;
}

bool TernaryCodingSystem::verifyTernaryTree(const std::vector<std::vector<int>> &tree, const std::string &data)
{
    int depth = tree.size() - 1;
    for (size_t i = 0; i < data.size() && i < tree[depth].size(); i++)
    {
        if (tree[depth][i] != (data[i] % 3))
            return false;
    }
    return true;
}
