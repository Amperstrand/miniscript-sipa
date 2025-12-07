// Test runner that loads tests from JSON
class JsonTestRunner {
    constructor() {
        this.testSuites = [];
        this.assertionFunctions = {
            'true': (value, expected) => value === true,
            'false': (value, expected) => value === false,
            'equal': (value, expected) => value === expected,
            'exists': (value, expected) => value != null,
            'not_exists': (value, expected) => value == null
        };
    }
    
    async loadTests(jsonUrl = 'tests.json') {
        try {
            const response = await fetch(jsonUrl);
            const data = await response.json();
            this.testSuites = data.testSuites;
            console.log(`Loaded ${this.testSuites.length} test suites`);
        } catch (error) {
            console.error('Failed to load tests:', error);
        }
    }
    
    evaluatePath(obj, path) {
        return path.split('.').reduce((current, key) => {
            if (key.includes('[')) {
                const [arrayKey, indexStr] = key.split('[');
                const index = parseInt(indexStr.replace(']', ''));
                return current[arrayKey][index];
            }
            return current[key];
        }, obj);
    }
    
    runAssertion(assertion, context) {
        const value = this.evaluatePath(context, assertion.value);
        const expected = assertion.expected;
        
        if (!this.assertionFunctions[assertion.type]) {
            throw new Error(`Unknown assertion type: ${assertion.type}`);
        }
        
        if (!this.assertionFunctions[assertion.type](value, expected)) {
            throw new Error(assertion.message + `\nExpected: ${JSON.stringify(expected)}\nActual: ${JSON.stringify(value)}`);
        }
    }
    
    async runTest(test) {
        // Call the appropriate function
        let result;
        switch (test.function) {
            case 'miniscript_to_json':
                result = miniscript_to_json(test.input);
                break;
            case 'miniscript_to_rete_json':
                result = miniscript_to_rete_json(test.input);
                break;
            case 'miniscript_to_complete_rete_json':
                result = miniscript_to_complete_rete_json(test.input);
                break;
            default:
                throw new Error(`Unknown function: ${test.function}`);
        }
        
        const context = { ast: JSON.parse(result) };
        
        // Run all assertions
        for (const assertion of test.assertions) {
            this.runAssertion(assertion, context);
        }
    }
    
    async runAllTests() {
        const results = [];
        
        for (const suite of this.testSuites) {
            console.log(`Running suite: ${suite.name}`);
            
            for (const test of suite.tests) {
                try {
                    await this.runTest(test);
                    results.push({ suite: suite.name, test: test.name, passed: true });
                    console.log(`  ✓ ${test.name}`);
                } catch (error) {
                    results.push({ suite: suite.name, test: test.name, passed: false, error: error.message });
                    console.log(`  ✗ ${test.name}: ${error.message}`);
                }
            }
        }
        
        return results;
    }
}

// Export for use in HTML
window.JsonTestRunner = JsonTestRunner;
