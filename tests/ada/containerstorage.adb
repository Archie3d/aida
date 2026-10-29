with Ada.Containers; use Ada.Containers;
with Ada.Containers.Storage;
with Ada.Text_IO;
procedure ContainerStorage is
    function Identity (Value : Integer) return Integer is begin return Value; end;
    package S is new Ada.Containers.Storage
        (Integer, Integer, Integer, Integer, Identity, Identity, False, False, True);
    use type S.Position;
    Data : S.Store;
    P : S.Position;
    Previous : Integer;
    Visited : Count_Type;
    function Check_Tree (Node : S.Position; Parent : S.Position) return Integer is
        Left, Right : Integer;
    begin
        if Node = null then return 0; end if;
        if Node.Parent /= Parent then raise Program_Error with "parent link"; end if;
        Left := Check_Tree (Node.Left, Node);
        if Node.Key.all <= Previous then raise Program_Error with "tree order"; end if;
        Previous := Node.Key.all;
        Visited := Visited + 1;
        Right := Check_Tree (Node.Right, Node);
        if Left - Right > 1 or Right - Left > 1 then raise Program_Error with "AVL balance"; end if;
        if Right > Left then Left := Right; end if;
        if Node.Height /= Left + 1 then raise Program_Error with "stored height"; end if;
        return Left + 1;
    end;
    procedure Check is
        Height : Integer;
        Cursor : S.Position := S.First (Data);
        Prev : S.Position := null;
        Count : Count_Type := 0;
    begin
        Previous := 0;
        Visited := 0;
        Height := Check_Tree (S.Root (Data), null);
        if Visited /= S.Length (Data) then raise Program_Error with "tree size"; end if;
        while Cursor /= null loop
            if Cursor.Prev /= Prev then raise Program_Error with "list link"; end if;
            if Prev /= null and then Prev.Key.all >= Cursor.Key.all then raise Program_Error; end if;
            Prev := Cursor;
            Cursor := Cursor.Next;
            Count := Count + 1;
        end loop;
        if Count /= Visited or Prev /= S.Last (Data) then raise Program_Error; end if;
    end;
begin
    for I in 1 .. 513 loop
        P := S.Create (I);
        S.Set_Key (P, I);
        S.Link (Data, null, P);
        Check;
    end loop;
    for I in 1 .. 513 loop
        declare
            Key : Integer := ((I * 73) mod 513) + 1;
        begin
            P := S.Root (Data);
            while P /= null and then P.Key.all /= Key loop
                if Key < P.Key.all then P := P.Left; else P := P.Right; end if;
            end loop;
            S.Delete (Data, P);
            Check;
        end;
    end loop;
    Ada.Text_IO.Put_Line ("container storage passed");
end ContainerStorage;
