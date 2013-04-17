function foo()
end

t1 = CustomTarget:new("t1", foo)
t2 = CustomTarget:new("t2", foo)
t3 = CustomTarget:new("t3", foo)
t4 = CustomTarget:new("t4", foo)
t5 = CustomTarget:new("t5", foo)
t6 = CustomTarget:new("t6", foo)

t1:addDependency(t2)
t1:addDependency(t3)

-- This test differ from parallel_build_2 the other of the dependencies
-- declaration, but should have the same result
t2:addDependency(t4)
t2:addDependency(t3)

t3:addDependency(t5)

t5:addDependency(t6)

-- Expected queue:
--   t4, t6
--   t5
--   t3
--   t2
--   t1
