#include "trie.h"
#include <assert.h>
#include <stdlib.h>

char** put(Trie* trie, int index)
{
	assert(index >= 0);

	if (trie->root == NULL)
		trie->root = calloc(1, sizeof(TrieNode));

	TrieNode* traverse = trie->root;

	while (index > 9)
	{
		int trieIndex = index % 10;
		index /= 10;

		if (!traverse->children[trieIndex])
			traverse->children[trieIndex] = calloc(1, sizeof(TrieNode));

		traverse = traverse->children[trieIndex];
	}

	if (!traverse->children[index])
		traverse->children[trieIndex] = calloc(1, sizeof(TrieNode));

	traverse = traverse->children[index];

	return &traverse->data;
}

char* get(Trie* trie, int index)
{	
	if (trie->root == NULL)
		return NULL;

	TrieNode* traverse = trie->root;

	if (index == 0)
	{
		if (traverse->children[0] == NULL)
			return NULL;
	
		return traverse->children[0]->data;
	}

	while (traverse && index)
	{
		traverse = traverse->children[index % 10];
		index /= 10;
	}

	if (!traverse)
		return NULL;

	return traverse->data;
}
/*
// private function to help del of entry in trie
void delNode(TrieNode* node, int index, int *status)
{
	if (node == NULL)	// can't delete the node if it is null
	{
		*status = 0;
		return;
	}

	if (
}

int del(Trie* trie, int index)
{
	int status = -1;

	if (index == 0)
	{
		if (trie->root == NULL)
			return 0;

		if (trie->root->children[0] == NULL)
			return 0;

		if (trie->root->children[0]->data != NULL)
		{
			
		}
	}

	delNode(trie->root, index, &status);

	assert(status != -1);

	return status;
}*/
