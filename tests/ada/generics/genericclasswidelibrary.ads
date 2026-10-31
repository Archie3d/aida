with GenericClasswideModel;
generic
    type T (<>) is new GenericClasswideModel.Root with private;
package GenericClasswideLibrary is
    function Score (Object : T) return Integer;
    function Clone (Object : T) return T;
    procedure Reset (Object : in out T);
    procedure Renamed_Reset (Object : in out T);
    generic
        type U (<>) is new T with private;
    function Nested_Score (Object : U) return Integer;
end GenericClasswideLibrary;
