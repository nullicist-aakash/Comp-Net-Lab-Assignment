/*******************************************************************************************************************************************************
	Aakash		-	2018B4A70887P
	Data structure	-	Trie
	Description	-	Trie is the data structure that is efficient in storing key value pairs and searching through it.
       				That's why it is used instead of hash table where we have the problem of collison

********************************************************************************************************************************************************/

#pragma once
#define TRIE_CHILD_COUNT 10

// Trie will be indexed for index 0 to 9, for each digit

typedef struct TrieNode
{
	char* data;
	struct TrieNode* children[TRIE_CHILD_COUNT];
} TrieNode;


typedef struct Trie{
	TrieNode* root;
} Trie;

char** put(Trie*, int);
char* get(Trie*, int);
int del(Trie*, int);

#endif
