with Ada.Containers; use Ada.Containers;
with Ada.Containers.Vectors;
with Ada.Containers.Doubly_Linked_Lists;
with Ada.Containers.Hashed_Maps;
with Ada.Containers.Ordered_Maps;
with Ada.Containers.Hashed_Sets;
with Ada.Containers.Ordered_Sets;
with Ada.Containers.Indefinite_Vectors;
with Ada.Containers.Indefinite_Doubly_Linked_Lists;
with Ada.Containers.Indefinite_Hashed_Maps;
with Ada.Containers.Indefinite_Ordered_Maps;
with Ada.Containers.Indefinite_Hashed_Sets;
with Ada.Containers.Indefinite_Ordered_Sets;
with Ada.Text_IO;
with Ada.Streams.Stream_IO;
with Ada.Strings.Unbounded; use Ada.Strings.Unbounded;
procedure StreamContainers is
    function Hash (Key : Integer) return Hash_Type is begin return Hash_Type (Key mod 7); end Hash;
    function Hash (Key : String) return Hash_Type is begin return Hash_Type (Key'Length mod 7); end Hash;
    package DV is new Ada.Containers.Vectors (Positive, Integer);
    package DL is new Ada.Containers.Doubly_Linked_Lists (Integer);
    package DHM is new Ada.Containers.Hashed_Maps (Integer, Integer, Hash, "=");
    package DOM is new Ada.Containers.Ordered_Maps (Integer, Integer);
    package DHS is new Ada.Containers.Hashed_Sets (Integer, Hash, "=");
    package DOS is new Ada.Containers.Ordered_Sets (Integer);
    package IV is new Ada.Containers.Indefinite_Vectors (Positive, String);
    package IL is new Ada.Containers.Indefinite_Doubly_Linked_Lists (String);
    package IHM is new Ada.Containers.Indefinite_Hashed_Maps (String, String, Hash, "=");
    package IOM is new Ada.Containers.Indefinite_Ordered_Maps (String, String);
    package IHS is new Ada.Containers.Indefinite_Hashed_Sets (String, Hash, "=");
    package IOS is new Ada.Containers.Indefinite_Ordered_Sets (String);
    A : DV.Vector; B : DL.List; C : DHM.Map; D : DOM.Map; E : DHS.Set; F : DOS.Set;
    G : IV.Vector; H : IL.List; I : IHM.Map; J : IOM.Map; K : IHS.Set; L : IOS.Set;
    File : Ada.Streams.Stream_IO.File_Type;
    Channel : Ada.Streams.Stream_IO.Stream_Access;
    Text : Unbounded_String := To_Unbounded_String ("dynamic text");
begin
    DV.Append (A, 1); DL.Append (B, 1); DHM.Insert (C, 1, 1); DOM.Insert (D, 1, 1);
    DHS.Insert (E, 1); DOS.Insert (F, 1);
    IV.Append (G, "hello"); IL.Append (H, "hello"); IHM.Insert (I, "hello", "world");
    IOM.Insert (J, "hello", "world"); IHS.Insert (K, "hello"); IOS.Insert (L, "hello");
    if DV.Element (A, 1) /= 1 or DL.Element (DL.First (B)) /= 1 or
        DHM.Element (C, 1) /= 1 or DOM.Element (D, 1) /= 1 or
        not DHS.Contains (E, 1) or not DOS.Contains (F, 1) or
        IV.Element (G, 1) /= "hello" or IL.Element (IL.First (H)) /= "hello" or
        IHM.Element (I, "hello") /= "world" or IOM.Element (J, "hello") /= "world" or
        not IHS.Contains (K, "hello") or not IOS.Contains (L, "hello") then
        raise Program_Error;
    end if;
    Ada.Streams.Stream_IO.Create (File, Ada.Streams.Stream_IO.Out_File, "streamcontainers.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    DV.Vector'Output (Channel, A);
    DL.List'Output (Channel, B);
    DHM.Map'Output (Channel, C);
    DOM.Map'Output (Channel, D);
    DHS.Set'Output (Channel, E);
    DOS.Set'Output (Channel, F);
    IV.Vector'Output (Channel, G);
    IL.List'Output (Channel, H);
    IHM.Map'Output (Channel, I);
    IOM.Map'Output (Channel, J);
    IHS.Set'Output (Channel, K);
    IOS.Set'Output (Channel, L);
    Unbounded_String'Output (Channel, Text);
    Ada.Streams.Stream_IO.Close (File);
    Ada.Streams.Stream_IO.Open (File, Ada.Streams.Stream_IO.In_File, "streamcontainers.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    declare
        Back : DV.Vector := DV.Vector'Input (Channel);
    begin
        if not DV."=" (Back, A) then raise Program_Error; end if;
    end;
    declare
        Back : DL.List := DL.List'Input (Channel);
    begin
        if not DL."=" (Back, B) then raise Program_Error; end if;
    end;
    declare
        Back : DHM.Map := DHM.Map'Input (Channel);
    begin
        if not DHM."=" (Back, C) then raise Program_Error; end if;
    end;
    declare
        Back : DOM.Map := DOM.Map'Input (Channel);
    begin
        if not DOM."=" (Back, D) then raise Program_Error; end if;
    end;
    declare
        Back : DHS.Set := DHS.Set'Input (Channel);
    begin
        if not DHS."=" (Back, E) then raise Program_Error; end if;
    end;
    declare
        Back : DOS.Set := DOS.Set'Input (Channel);
    begin
        if not DOS."=" (Back, F) then raise Program_Error; end if;
    end;
    declare
        Back : IV.Vector := IV.Vector'Input (Channel);
    begin
        if not IV."=" (Back, G) then raise Program_Error; end if;
    end;
    declare
        Back : IL.List := IL.List'Input (Channel);
    begin
        if not IL."=" (Back, H) then raise Program_Error; end if;
    end;
    declare
        Back : IHM.Map := IHM.Map'Input (Channel);
    begin
        if not IHM."=" (Back, I) then raise Program_Error; end if;
    end;
    declare
        Back : IOM.Map := IOM.Map'Input (Channel);
    begin
        if not IOM."=" (Back, J) then raise Program_Error; end if;
    end;
    declare
        Back : IHS.Set := IHS.Set'Input (Channel);
    begin
        if not IHS."=" (Back, K) then raise Program_Error; end if;
    end;
    declare
        Back : IOS.Set := IOS.Set'Input (Channel);
    begin
        if not IOS."=" (Back, L) then raise Program_Error; end if;
    end;
    if To_String (Unbounded_String'Input (Channel)) /= "dynamic text" then
        raise Program_Error;
    end if;
    Ada.Streams.Stream_IO.Delete (File);
    Ada.Text_IO.Put_Line ("streamcontainers: passed");
end StreamContainers;
