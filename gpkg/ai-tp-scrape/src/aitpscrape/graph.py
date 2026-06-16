"""
GraphExecutor Module - Core graph-based execution engine
"""

from typing import List, Dict, Any, Optional, Callable, Tuple
from dataclasses import dataclass, field
from abc import ABC, abstractmethod
from enum import Enum
import time
import logging


class NodeType(Enum):
    """Node type enumeration"""
    FETCH = "fetch"
    PARSE = "parse"
    FILTER = "filter"
    TRANSFORM = "transform"
    OUTPUT = "output"
    CONDITIONAL = "conditional"
    CUSTOM = "custom"


@dataclass
class NodeResult:
    """Result from a node execution"""
    node_name: str
    success: bool
    output: Any = None
    error: Optional[str] = None
    execution_time: float = 0.0
    metadata: Dict[str, Any] = field(default_factory=dict)


@dataclass
class ExecutionContext:
    """Context passed during graph execution"""
    state: Dict[str, Any]
    history: List[NodeResult]
    start_time: float = field(default_factory=time.time)
    
    def get(self, key: str, default: Any = None) -> Any:
        """Get value from state with default"""
        return self.state.get(key, default)
    
    def set(self, key: str, value: Any) -> None:
        """Set value in state"""
        self.state[key] = value
    
    def update(self, **kwargs) -> None:
        """Update multiple values in state"""
        self.state.update(kwargs)
    
    @property
    def elapsed_time(self) -> float:
        """Get elapsed time since start"""
        return time.time() - self.start_time


class BaseNode(ABC):
    """
    Abstract base class for nodes in the execution graph.
    
    Attributes:
        name (str): Unique name of the node
        node_type (NodeType): Type of the node
        input_keys (List[str]): Keys required from state
        output_key (str): Key to write output to
        
    Example:
        >>> class MyNode(BaseNode):
        ...     def execute(self, ctx: ExecutionContext) -> Any:
        ...         return {"result": ctx.get("input") * 2}
        >>> node = MyNode("double", NodeType.TRANSFORM, ["input"], "output")
    """
    
    def __init__(
        self,
        name: str,
        node_type: NodeType,
        input_keys: List[str],
        output_key: str,
    ):
        self.name = name
        self.node_type = node_type
        self.input_keys = input_keys
        self.output_key = output_key
        self.logger = self._setup_logger()
    
    def _setup_logger(self) -> logging.Logger:
        """Setup logger for this node"""
        logger = logging.getLogger(f"aitpscrape.graph.{self.name}")
        if not logger.handlers:
            handler = logging.StreamHandler()
            handler.setFormatter(logging.Formatter(
                "%(asctime)s - %(name)s - %(levelname)s - %(message)s"
            ))
            logger.addHandler(handler)
            logger.setLevel(logging.INFO)
        return logger
    
    def validate_inputs(self, ctx: ExecutionContext) -> Tuple[bool, Optional[str]]:
        """
        Validate that required inputs are available.
        
        Args:
            ctx: Execution context
            
        Returns:
            Tuple of (is_valid, error_message)
        """
        for key in self.input_keys:
            if key not in ctx.state:
                return False, f"Required input '{key}' not found in state"
        return True, None
    
    def execute(self, ctx: ExecutionContext) -> Any:
        """
        Execute the node logic.
        
        Args:
            ctx: Execution context
            
        Returns:
            Any: Output to be stored in state
        """
        is_valid, error = self.validate_inputs(ctx)
        if not is_valid:
            raise ValueError(error)
        
        return self._execute(ctx)
    
    @abstractmethod
    def _execute(self, ctx: ExecutionContext) -> Any:
        """
        Internal execution logic to be implemented by subclasses.
        
        Args:
            ctx: Execution context
            
        Returns:
            Any: Output to be stored in state
        """
        pass


class GraphExecutor:
    """
    Graph-based execution engine for orchestrating scraping workflows.
    
    Features:
    - Node-based execution flow
    - Conditional branching
    - State management
    - Error handling and recovery
    - Execution tracing
    
    Attributes:
        name (str): Name of the graph
        nodes (Dict[str, BaseNode]): Dictionary of nodes by name
        edges (List[Tuple[str, str]]): Edges representing execution flow
        
    Example:
        >>> executor = GraphExecutor("scraper")
        >>> executor.add_node(fetch_node)
        >>> executor.add_node(parse_node)
        >>> executor.add_edge("fetch", "parse")
        >>> result = executor.execute({"url": "https://example.com"})
        >>> print(result.state)
    """
    
    def __init__(self, name: str = "GraphExecutor"):
        self.name = name
        self.nodes: Dict[str, BaseNode] = {}
        self.edges: List[Tuple[str, str]] = []
        self.conditionals: Dict[str, Callable] = {}
        self.logger = self._setup_logger()
        
        self._execution_history: List[NodeResult] = []
        self._max_iterations = 1000
    
    def _setup_logger(self) -> logging.Logger:
        """Setup logger for this executor"""
        logger = logging.getLogger(f"aitpscrape.graph.{self.name}")
        if not logger.handlers:
            handler = logging.StreamHandler()
            handler.setFormatter(logging.Formatter(
                "%(asctime)s - %(name)s - %(levelname)s - %(message)s"
            ))
            logger.addHandler(handler)
            logger.setLevel(logging.INFO)
        return logger
    
    def add_node(self, node: BaseNode) -> "GraphExecutor":
        """
        Add a node to the graph.
        
        Args:
            node: Node to add
            
        Returns:
            self for chaining
        """
        if node.name in self.nodes:
            raise ValueError(f"Node '{node.name}' already exists")
        
        self.nodes[node.name] = node
        self.logger.info(f"Added node: {node.name} (type: {node.node_type.value})")
        return self
    
    def add_edge(self, from_node: str, to_node: str) -> "GraphExecutor":
        """
        Add an edge between nodes.
        
        Args:
            from_node: Name of source node
            to_node: Name of target node
            
        Returns:
            self for chaining
        """
        if from_node not in self.nodes:
            raise ValueError(f"Source node '{from_node}' not found")
        if to_node not in self.nodes:
            raise ValueError(f"Target node '{to_node}' not found")
        
        self.edges.append((from_node, to_node))
        self.logger.debug(f"Added edge: {from_node} -> {to_node}")
        return self
    
    def add_conditional(
        self,
        from_node: str,
        condition: Callable[[ExecutionContext], bool],
        true_node: str,
        false_node: str,
    ) -> "GraphExecutor":
        """
        Add a conditional edge from a node.
        
        Args:
            from_node: Name of source node
            condition: Function that takes context and returns bool
            true_node: Node to execute if condition is True
            false_node: Node to execute if condition is False
            
        Returns:
            self for chaining
        """
        if from_node not in self.nodes:
            raise ValueError(f"Source node '{from_node}' not found")
        
        self.conditionals[from_node] = {
            "condition": condition,
            "true": true_node,
            "false": false_node,
        }
        return self
    
    def set_entry_point(self, node_name: str) -> "GraphExecutor":
        """
        Set the entry point node.
        
        Args:
            node_name: Name of entry point node
            
        Returns:
            self for chaining
        """
        if node_name not in self.nodes:
            raise ValueError(f"Entry point '{node_name}' not found")
        
        self._entry_point = node_name
        return self
    
    def execute(self, initial_state: Dict[str, Any]) -> ExecutionContext:
        """
        Execute the graph from the entry point.
        
        Args:
            initial_state: Initial state dictionary
            
        Returns:
            ExecutionContext with final state and history
        """
        if not hasattr(self, "_entry_point"):
            raise ValueError("Entry point not set. Call set_entry_point() first.")
        
        ctx = ExecutionContext(
            state=initial_state.copy(),
            history=[],
        )
        
        self.logger.info(f"Starting graph execution: {self.name}")
        
        try:
            self._execute_from(ctx, self._entry_point)
            self.logger.info(
                f"Graph execution completed in {ctx.elapsed_time:.2f}s"
            )
        except Exception as e:
            self.logger.error(f"Graph execution failed: {str(e)}")
            raise
        
        return ctx
    
    def _execute_from(self, ctx: ExecutionContext, node_name: str) -> None:
        """
        Execute starting from a specific node.
        
        Args:
            ctx: Execution context
            node_name: Name of starting node
        """
        iteration = 0
        current_node = node_name
        
        while current_node is not None and iteration < self._max_iterations:
            iteration += 1
            
            if current_node not in self.nodes:
                raise ValueError(f"Node '{current_node}' not found")
            
            node = self.nodes[current_node]
            
            self.logger.debug(f"Executing node: {current_node}")
            
            start_time = time.time()
            try:
                output = node.execute(ctx)
                
                if output is not None:
                    ctx.set(node.output_key, output)
                
                execution_time = time.time() - start_time
                
                result = NodeResult(
                    node_name=current_node,
                    success=True,
                    output=output,
                    execution_time=execution_time,
                )
                
            except Exception as e:
                execution_time = time.time() - start_time
                self.logger.error(f"Node '{current_node}' failed: {str(e)}")
                
                result = NodeResult(
                    node_name=current_node,
                    success=False,
                    error=str(e),
                    execution_time=execution_time,
                )
                
                ctx.history.append(result)
                raise
            
            ctx.history.append(result)
            
            next_node = self._get_next_node(ctx, current_node)
            current_node = next_node
        
        if iteration >= self._max_iterations:
            raise RecursionError(
                f"Maximum iterations ({self._max_iterations}) exceeded"
            )
    
    def _get_next_node(self, ctx: ExecutionContext, current_node: str) -> Optional[str]:
        """
        Determine the next node to execute.
        
        Args:
            ctx: Execution context
            current_node: Current node name
            
        Returns:
            Name of next node or None to stop
        """
        if current_node in self.conditionals:
            cond = self.conditionals[current_node]
            if cond["condition"](ctx):
                return cond["true"]
            else:
                return cond["false"]
        
        outgoing_edges = [to_n for (from_n, to_n) in self.edges if from_n == current_node]
        
        if not outgoing_edges:
            return None
        
        return outgoing_edges[0] if len(outgoing_edges) == 1 else None
    
    def get_execution_summary(self, ctx: ExecutionContext) -> Dict[str, Any]:
        """
        Get a summary of the execution.
        
        Args:
            ctx: Execution context
            
        Returns:
            Dictionary with execution summary
        """
        total_time = sum(r.execution_time for r in ctx.history)
        success_count = sum(1 for r in ctx.history if r.success)
        error_count = len(ctx.history) - success_count
        
        return {
            "total_nodes": len(ctx.history),
            "successful_nodes": success_count,
            "failed_nodes": error_count,
            "total_time": total_time,
            "state_keys": list(ctx.state.keys()),
        }
    
    def visualize(self) -> str:
        """
        Generate a simple text visualization of the graph.
        
        Returns:
            String representation of the graph
        """
        lines = [f"Graph: {self.name}", "=" * 50]
        
        for name, node in self.nodes.items():
            node_type = node.node_type.value
            inputs = ", ".join(node.input_keys) or "(none)"
            output = node.output_key or "(none)"
            
            lines.append(f"\n[{name}]")
            lines.append(f"  Type: {node_type}")
            lines.append(f"  Inputs: {inputs}")
            lines.append(f"  Output: {output}")
        
        if self.edges:
            lines.append("\nEdges:")
            for from_n, to_n in self.edges:
                lines.append(f"  {from_n} -> {to_n}")
        
        return "\n".join(lines)
