package AnonymousArrayData is
    type Cell is record
        Value : Integer;
    end record;
    Width : Integer := 2;
    Items : array (1 .. Width) of access Cell := (others => new Cell);
end AnonymousArrayData;
