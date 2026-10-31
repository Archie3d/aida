with GenericAbstractModel;
generic
    type T is abstract new GenericAbstractModel.Root with private;
    with function Measure (Object : T; Extra : Integer := 5) return Integer is abstract Read;
    with function Read (Object : T; Extra : Integer := 6) return Integer is abstract <>;
    with procedure Change (Object : in out T; Amount : Integer := 4) is abstract <>;
package GenericAbstractLibrary is
    function Score (Object : T'Class) return Integer;
    function Box_Score (Object : T'Class) return Integer;
    procedure Update (Object : in out T'Class);
    generic
        with function Nested (Object : T; Extra : Integer := 8) return Integer is abstract Measure;
    function Nested_Score (Object : T'Class) return Integer;
end GenericAbstractLibrary;
