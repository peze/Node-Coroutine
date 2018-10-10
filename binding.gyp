{
  "targets": [
    {
      "target_name": "node-coroutine",
      "sources": ["src/context_swap.S","src/context.cpp", "src/coroutine.cpp","src/main.cpp",]
    }
  ],
  'cflags': [
	  '-Wall',
	  '-O3',
	  '-std=c++11'
  ]
}