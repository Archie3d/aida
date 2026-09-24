procedure GenericCapacityErrors is
    generic
        capacity : Positive;
    package Bad is
        subtype Count is Natural range 0 .. capacity;
        type Buffer is record
            data : String (1 .. capacity) := (others => False);
        end record;
    end Bad;
begin
    null;
end GenericCapacityErrors;
