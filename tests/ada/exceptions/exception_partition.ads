package Exception_Partition is
    Failure : exception;
    Message : constant String (1 .. 1000) := (others => 'p');
end Exception_Partition;
