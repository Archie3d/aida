with Ada.Containers; use Ada.Containers;
with Ada.Containers.Indefinite_Doubly_Linked_Lists;
with Ada.Containers.Indefinite_Hashed_Maps;
with Ada.Strings.Unbounded; use Ada.Strings.Unbounded;
with Ada.Unchecked_Deallocation;
with Ada.Text_IO;
procedure ContainerTasking is
    function Hash (Key : String) return Hash_Type is begin return Hash_Type (Key'Length mod 3); end;
    package Lists is new Ada.Containers.Indefinite_Doubly_Linked_Lists (String);
    package Maps is new Ada.Containers.Indefinite_Hashed_Maps (String, Unbounded_String, Hash, "=");
    type Box is record
        List : Lists.List;
        Map : Maps.Map;
    end record;
    type Link is access Box;
    procedure Free is new Ada.Unchecked_Deallocation (Box, Link);
    protected Channel is
        entry Put (Item : Link);
        entry Take (Item : out Link);
        procedure Done (Good : Boolean);
        function Passed return Boolean;
    private
        Pending : Link;
        Completed : Natural := 0;
        Errors : Natural := 0;
    end Channel;
    protected body Channel is
        entry Put (Item : Link) when Pending = null is
        begin
            Pending := Item;
        end;
        entry Take (Item : out Link) when Pending /= null is
        begin
            Item := Pending;
            Pending := null;
        end;
        procedure Done (Good : Boolean) is
        begin
            Completed := Completed + 1;
            if not Good then Errors := Errors + 1; end if;
        end;
        function Passed return Boolean is begin return Completed = 4 and Errors = 0; end;
    end Channel;
    procedure Exercise is
        task type Producer;
        task type Consumer;
        task body Producer is
            Item : Link;
        begin
            for I in 1 .. 20 loop
                Item := new Box;
                Lists.Append (Item.List, "allocated in producer");
                Maps.Insert (Item.Map, "key", To_Unbounded_String ("value"));
                Channel.Put (Item);
                Item := null;
            end loop;
            Channel.Done (True);
        exception
            when others => Channel.Done (False);
        end;
        task body Consumer is
            Item : Link;
        begin
            for I in 1 .. 20 loop
                Channel.Take (Item);
                declare
                    Copy : Box := Item.all;
                begin
                    if Lists.First_Element (Copy.List) /= "allocated in producer" or
                        Maps.Element (Copy.Map, "key") /= "value" then raise Program_Error; end if;
                    Lists.Clear (Copy.List);
                    Maps.Replace (Copy.Map, "key", To_Unbounded_String ("consumer"));
                    Free (Item);
                end;
            end loop;
            Channel.Done (True);
        exception
            when others => Channel.Done (False);
        end;
        type Producers is array (1 .. 2) of Producer;
        type Consumers is array (1 .. 2) of Consumer;
        Senders : Producers;
        Receivers : Consumers;
    begin
        null;
    end;
begin
    Exercise;
    if not Channel.Passed then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("container tasking passed");
end ContainerTasking;
