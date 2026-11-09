import * as mock from './mockApi.js';
import * as real from './realApi.js';
export const api = import.meta.env.VITE_USE_MOCKS !== 'false' ? mock : real;
