-- Gives back the storage an allocator took.  Nothing checks that no other
-- access value still designates the object, which is what "unchecked" says:
-- reaching through one of those afterwards is the program's own doing.

generic
    type Object is limited private;
    -- Formal access types are not supported yet. The compiler checks this
    -- predefined unit's Object/Name relationship explicitly at instantiation.
    type Name is private;

procedure Ada.Unchecked_Deallocation (X : in out Name);
