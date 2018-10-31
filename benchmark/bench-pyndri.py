from multiprocessing import Pool
import pyndri
import json
import sys

index = pyndri.Index(sys.argv[1])

lm_query_env = pyndri.QueryEnvironment(
                                 index, rules=('method:dirichlet,mu:2000',))

def run_query(query):
    lm_query_env.query(query)


p = Pool(20)

queries = json.load(open(sys.argv[2]))

query_load = []
for query in queries:
    query_load.append(queries[query]);

p.map(run_query, query_load)
p.close()
p.join()