from collections import deque

file1 = open('A Star Search\Input_file.txt','r')
inp = file1.readlines()

# Creating two dictionaries. One is for the adjacency list and the other is for the heuristic values.
dict1 = {}
dict2 = {}
for i in inp:
    arr1 = i[:-1].split(" ")
    arr2 = arr1[2:]
    dict1[arr1[0]] = []
    for j in range(len(arr2)-1):
      if j%2==0:
        dict1[arr1[0]].append((arr2[j],int(arr2[j+1])))
    dict2[arr1[0]]=int(arr1[1])

# The class Graph is a class that has two attributes: adjacency_list and heuristic_val.  
# The class Graph has two methods: get_neighbors and h. 
# The get_neighbors method takes in a vertex and returns a list of vertices that are adjacent to the input vertex. 
# The h method takes in a vertex and returns the heuristic value of that vertex.
class Graph:

    def __init__(self, adjacency_list,heuristic_val):
        """
        The function takes in a graph and a heuristic value and returns the neighbors of a node and the
        heuristic value of a node
        :param adjacency_list: a dictionary of lists, where the key is the node and the value is a list
        of tuples, where each tuple is a node and the weight of the edge between the two nodes
        :param heuristic_val: A dictionary of the heuristic values of each node
        """
        self.adjacency_list = adjacency_list
        self.heuristic_val = heuristic_val

    def get_neighbors(self, v):
        return self.adjacency_list[v]

    def h(self, n):
        return self.heuristic_val[n]

    def a_star_algorithm(self, start_node, stop_node):
        # This is the part of the code that is initializing the open list, closed list, g, and
        # parents.
        open_list = set([start_node])
        closed_list = set([])

        g = {}
        g[start_node] = 0
        parents = {}
        parents[start_node] = start_node

        while len(open_list) > 0:
            n = None
            # This is the part of the code that is finding the node with the lowest f value.
            for v in open_list:
                if n == None or g[v] + self.h(v) < g[n] + self.h(n):
                    n = v

            if n == None:
                print('Path does not exist!')
                return None

            if n == stop_node:
                reconst_path = []

                while parents[n] != n:
                    reconst_path.append(n)
                    n = parents[n]

                reconst_path.append(start_node)
                reconst_path.reverse()

                dis = 0
                # This is the part of the code that calculates the distance of the path.
                for x in range(len(reconst_path)-1):
                  for (y,z) in self.adjacency_list[reconst_path[x]]:
                    if y==reconst_path[x+1]:
                      dis+=z
                return (f"Distance: {dis} Km \nPath: {' ->' .join(reconst_path)}")
            # This is the main part of the A* algorithm. It is checking if the node is in the open
            # list or the closed list. If it is not in either of the lists, then it is added to the
            # open list. If it is in the open list, then it checks if the distance of the node is
            # greater than the distance of the node plus the weight of the edge. If it is, then the
            # distance of the node is updated. If it is in the closed list, then it is removed from
            # the closed list and added to the open list.
            for (m, weight) in self.get_neighbors(n):

                if m not in open_list and m not in closed_list:
                    open_list.add(m)
                    parents[m] = n
                    g[m] = g[n] + weight

                else:
                    if g[m] > g[n] + weight:
                        g[m] = g[n] + weight
                        parents[m] = n

                        if m in closed_list:
                            closed_list.remove(m)
                            open_list.add(m)
            open_list.remove(n)
            closed_list.add(n)

        print('Path does not exist!')
        return None
# Taking the input from the user and then printing the output.
start_node = input('Start node: ')
end_node = input('Destination: ')
graph1 = Graph(dict1,dict2)
print(graph1.a_star_algorithm(start_node, end_node))